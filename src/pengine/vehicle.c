#include "vehicle.h"

#include <math.h>
#include <string.h>

#define STEP_MAX (1.0f / 240.0f)

//the wall pushes back this much of the speed it was run into
//past the end of its travel a spring is this many times as stiff
#define BUMP_STOP_STIFFNESS 10.0f

#define WALL_RESTITUTION 0.2f

static const vec3 BODY_LEFT = {1, 0, 0};
static const vec3 BODY_FORWARD = {0, 1, 0};
static const vec3 BODY_UP = {0, 0, 1};
static const vec3 WORLD_UP = {0, 0, 1};

void pe_vehicle_init(PVehicle *vehicle, float mass, const vec3 half_extents) {
  memset(vehicle, 0, sizeof(*vehicle));
  vehicle->mass = mass;

  float x = 2 * half_extents[0], y = 2 * half_extents[1], z = 2 * half_extents[2];
  vehicle->inertia[0] = mass * (y * y + z * z) / 12;
  vehicle->inertia[1] = mass * (x * x + z * z) / 12;
  vehicle->inertia[2] = mass * (x * x + y * y) / 12;

  glm_vec3_copy((vec3){0, 0, -9.81f}, vehicle->gravity);
  vehicle->linear_drag = 0.05f;
  vehicle->angular_drag = 1.5f;
  vehicle->brake_friction = 1.0f;
  vehicle->rolling_resistance = 0.03f;
  glm_quat_identity(vehicle->orientation);
  vehicle->sphere_radius = half_extents[0];
}

bool pe_vehicle_add_wheel(PVehicle *vehicle, const PVehicleWheel *wheel) {
  if (vehicle->wheel_count >= PE_VEHICLE_WHEELS_MAX)
    return false;
  PVehicleWheel *added = &vehicle->wheels[vehicle->wheel_count++];
  *added = *wheel;
  added->suspension_length = wheel->rest_length;
  return true;
}

bool pe_vehicle_add_sphere(PVehicle *vehicle, const vec3 centre) {
  if (vehicle->sphere_count >= PE_VEHICLE_BODY_SPHERES_MAX)
    return false;
  glm_vec3_copy((float *)centre, vehicle->sphere_centres[vehicle->sphere_count++]);
  return true;
}

void pe_vehicle_place(PVehicle *vehicle, const vec3 position,
                      float heading_degrees) {
  glm_vec3_copy((float *)position, vehicle->position);
  glm_quat(vehicle->orientation, glm_rad(heading_degrees), 0, 0, 1);
  glm_vec3_zero(vehicle->linear_velocity);
  glm_vec3_zero(vehicle->angular_velocity);
}

static void to_world(const PVehicle *vehicle, const vec3 local, vec3 out) {
  glm_quat_rotatev((float *)vehicle->orientation, (float *)local, out);
}

static void velocity_at(const PVehicle *vehicle, const vec3 offset, vec3 out) {
  glm_vec3_cross((float *)vehicle->angular_velocity, (float *)offset, out);
  glm_vec3_add(out, (float *)vehicle->linear_velocity, out);
}

//the angular velocity a torque adds over seconds, worked out in the body's own
//axes where the inertia is diagonal
static void add_torque(PVehicle *vehicle, const vec3 torque, float seconds) {
  versor inverse;
  glm_quat_inv(vehicle->orientation, inverse);

  vec3 local;
  glm_quat_rotatev(inverse, (float *)torque, local);
  for (int i = 0; i < 3; i++)
    local[i] = local[i] / vehicle->inertia[i] * seconds;

  vec3 world;
  to_world(vehicle, local, world);
  glm_vec3_add(vehicle->angular_velocity, world, vehicle->angular_velocity);
}

static void push_at(PVehicle *vehicle, const vec3 force, const vec3 offset,
                    float seconds) {
  vec3 torque;
  glm_vec3_muladds((float *)force, seconds / vehicle->mass,
                   vehicle->linear_velocity);
  glm_vec3_cross((float *)offset, (float *)force, torque);
  add_torque(vehicle, torque, seconds);
}

//the ray of a wheel runs down the body's own up, and finds how long its
//suspension is, or leaves the wheel in the air
static void probe_wheel(PVehicleWheel *wheel, const PCollisionMesh *ground,
                        const vec3 up, const vec3 mount) {
  float min_length = fmaxf(wheel->rest_length - wheel->travel, 0);
  float max_length = wheel->rest_length + wheel->travel;

  vec3 down;
  glm_vec3_negate_to((float *)up, down);

  float distance;
  wheel->grounded = ground != NULL &&
                    pe_collision_mesh_floor(ground, mount, down,
                                            max_length + wheel->radius,
                                            &distance);
  wheel->overtravel = 0;
  if (!wheel->grounded) {
    wheel->suspension_length = max_length;
    return;
  }

  float length = distance - wheel->radius;
  wheel->overtravel = fmaxf(min_length - length, 0);
  wheel->suspension_length = glm_clamp(length, min_length, max_length);
}

static float spring_force(const PVehicle *vehicle, const PVehicleWheel *wheel,
                          const vec3 up, const vec3 offset) {
  vec3 velocity;
  velocity_at(vehicle, offset, velocity);

  float share = vehicle->mass / vehicle->wheel_count;
  float damping = 2 * wheel->damping_ratio * sqrtf(wheel->stiffness);
  float spring =
      wheel->stiffness * (wheel->rest_length - wheel->suspension_length +
                          BUMP_STOP_STIFFNESS * wheel->overtravel);
  float force = share * (spring - damping * glm_vec3_dot(velocity, (float *)up));
  return fmaxf(force, 0);
}

static u32 traction_count(const PVehicle *vehicle) {
  u32 count = 0;
  for (u32 i = 0; i < vehicle->wheel_count; i++)
    if (vehicle->wheels[i].traction)
      count++;
  return count;
}

//the tyre's force along the ground: what the driver asks for along the wheel
//and what stops it sliding across, both held to the load times the friction
//the tyre has, together
static void tyre_force(const PVehicle *vehicle, const PVehicleWheel *wheel,
                       const vec3 contact_offset, const vec3 along,
                       const vec3 across, float seconds, vec3 out) {
  vec3 velocity;
  velocity_at(vehicle, contact_offset, velocity);
  float speed_along = glm_vec3_dot(velocity, (float *)along);
  float speed_across = glm_vec3_dot(velocity, (float *)across);

  float share = vehicle->mass / vehicle->wheel_count;
  float limit = wheel->friction_slip * wheel->load;

  float sideways = -speed_across * share / seconds;

  float lengthways = 0;
  if (wheel->traction)
    lengthways = vehicle->engine_force / traction_count(vehicle);
  float stopping = -speed_along * share / seconds;
  float braking = glm_clamp(stopping, -vehicle->brake * vehicle->brake_friction * limit,
                            vehicle->brake * vehicle->brake_friction * limit);
  lengthways += braking;

  float resistance = vehicle->rolling_resistance * wheel->load;
  lengthways += glm_clamp(stopping, -resistance, resistance);

  float total = hypotf(sideways, lengthways);
  if (total > limit && total > 0) {
    sideways *= limit / total;
    lengthways *= limit / total;
  }

  glm_vec3_scale((float *)along, lengthways, out);
  glm_vec3_muladds((float *)across, sideways, out);
}

static void wheel_axes(const PVehicle *vehicle, const PVehicleWheel *wheel,
                       vec3 along, vec3 across) {
  float steer = wheel->steer_angle;
  to_world(vehicle, (vec3){sinf(steer), cosf(steer), 0}, along);
  to_world(vehicle, (vec3){cosf(steer), -sinf(steer), 0}, across);
}

static void drive_wheel(PVehicle *vehicle, PVehicleWheel *wheel,
                        const PCollisionMesh *ground, const vec3 up,
                        float seconds) {
  wheel->steer_angle = wheel->steering ? vehicle->steering : 0;

  vec3 mount_offset;
  to_world(vehicle, wheel->mount, mount_offset);
  vec3 mount;
  glm_vec3_add(mount_offset, vehicle->position, mount);

  probe_wheel(wheel, ground, up, mount);
  if (!wheel->grounded) {
    wheel->load = 0;
    return;
  }

  vec3 contact_offset;
  glm_vec3_muladds((float *)up, -(wheel->suspension_length + wheel->radius),
                   mount_offset);
  glm_vec3_copy(mount_offset, contact_offset);

  wheel->load = spring_force(vehicle, wheel, up, contact_offset);
  vec3 lift;
  glm_vec3_scale((float *)up, wheel->load, lift);
  push_at(vehicle, lift, contact_offset, seconds);

  vec3 along, across, force;
  wheel_axes(vehicle, wheel, along, across);
  tyre_force(vehicle, wheel, contact_offset, along, across, seconds, force);

  //the tyre pushes the body along the ground at the contact, and the part of
  //it that is sideways is moved up toward the centre of mass by roll_influence
  float height = glm_vec3_dot(contact_offset, (float *)up);
  vec3 sideways_offset;
  glm_vec3_copy(contact_offset, sideways_offset);
  glm_vec3_muladds((float *)up, -height * (1 - wheel->roll_influence),
                   sideways_offset);

  float forward = glm_vec3_dot(force, along);
  vec3 lengthways, sideways;
  glm_vec3_scale(along, forward, lengthways);
  glm_vec3_sub(force, lengthways, sideways);
  push_at(vehicle, lengthways, contact_offset, seconds);
  push_at(vehicle, sideways, sideways_offset, seconds);

  vec3 velocity;
  velocity_at(vehicle, contact_offset, velocity);
  wheel->spin_angle += glm_vec3_dot(velocity, along) / wheel->radius * seconds;
}

static void keep_out_of_walls(PVehicle *vehicle, const PCollisionMesh *ground) {
  for (u32 i = 0; i < vehicle->sphere_count; i++) {
    vec3 offset, centre, moved;
    to_world(vehicle, vehicle->sphere_centres[i], offset);
    glm_vec3_add(vehicle->position, offset, centre);
    glm_vec3_copy(centre, moved);

    if (!pe_collision_mesh_push_out(ground, moved, vehicle->sphere_radius,
                                    WORLD_UP))
      continue;

    vec3 push;
    glm_vec3_sub(moved, centre, push);
    vec3 normal;
    glm_vec3_normalize_to(push, normal);
    glm_vec3_add(vehicle->position, push, vehicle->position);

    float into = glm_vec3_dot(vehicle->linear_velocity, normal);
    if (into >= 0)
      continue;
    glm_vec3_muladds(normal, -into * (1 + WALL_RESTITUTION),
                     vehicle->linear_velocity);
    vehicle->impact_speed = fmaxf(vehicle->impact_speed, -into);
  }
}

static void integrate(PVehicle *vehicle, float seconds) {
  glm_vec3_muladds(vehicle->gravity, seconds, vehicle->linear_velocity);
  glm_vec3_scale(vehicle->linear_velocity,
                 fmaxf(1 - vehicle->linear_drag * seconds, 0),
                 vehicle->linear_velocity);
  glm_vec3_scale(vehicle->angular_velocity,
                 fmaxf(1 - vehicle->angular_drag * seconds, 0),
                 vehicle->angular_velocity);

  glm_vec3_muladds(vehicle->linear_velocity, seconds, vehicle->position);

  versor spin = {vehicle->angular_velocity[0], vehicle->angular_velocity[1],
                 vehicle->angular_velocity[2], 0};
  versor change;
  glm_quat_mul(spin, vehicle->orientation, change);
  glm_vec4_muladds(change, 0.5f * seconds, vehicle->orientation);
  glm_quat_normalize(vehicle->orientation);
}

static void step_once(PVehicle *vehicle, const PCollisionMesh *ground,
                      float seconds) {
  vec3 up;
  to_world(vehicle, BODY_UP, up);

  for (u32 i = 0; i < vehicle->wheel_count; i++)
    drive_wheel(vehicle, &vehicle->wheels[i], ground, up, seconds);

  integrate(vehicle, seconds);
  if (ground != NULL)
    keep_out_of_walls(vehicle, ground);
}

void pe_vehicle_step(PVehicle *vehicle, const PCollisionMesh *ground,
                     float seconds) {
  vehicle->impact_speed = 0;
  u32 steps = (u32)ceilf(seconds / STEP_MAX);
  if (steps == 0)
    return;
  for (u32 i = 0; i < steps; i++)
    step_once(vehicle, ground, seconds / steps);
}

void pe_vehicle_transform(const PVehicle *vehicle, mat4 out) {
  glm_quat_mat4((float *)vehicle->orientation, out);
  glm_vec3_copy((float *)vehicle->position, out[3]);
}

void pe_vehicle_wheel_transform(const PVehicle *vehicle, u32 index, mat4 out) {
  const PVehicleWheel *wheel = &vehicle->wheels[index];

  pe_vehicle_transform(vehicle, out);
  glm_translate(out, (vec3){wheel->mount[0], wheel->mount[1],
                            wheel->mount[2] - wheel->suspension_length});
  glm_rotate(out, -wheel->steer_angle, (float *)BODY_UP);
  glm_rotate(out, -wheel->spin_angle, (float *)BODY_LEFT);
}

float pe_vehicle_forward_speed(const PVehicle *vehicle) {
  vec3 forward;
  to_world(vehicle, BODY_FORWARD, forward);
  return glm_vec3_dot((float *)vehicle->linear_velocity, forward);
}
