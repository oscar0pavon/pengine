#ifndef PE_VEHICLE_H
#define PE_VEHICLE_H

#include "numbers.h"
#include "terrain/terrain_collision.h"

#include <cglm/cglm.h>

#define PE_VEHICLE_WHEELS_MAX 8
#define PE_VEHICLE_BODY_SPHERES_MAX 4

//a body's own axes: Y forward, Z up and X to the left, which is the way the
//engine's left handed world has it. the world's up is Z too, so a car whose
//mesh is exported with Z up needs no change of axes
typedef struct PVehicleWheel {
  //where the suspension is fixed to the body, in the body's axes. the wheel's
  //hub hangs below it by suspension_length
  vec3 mount;
  float radius;

  //the length the spring settles at when it holds its share of the weight, and
  //how far it may go either side of that
  float rest_length;
  float travel;

  //stiffness is per unit of mass the wheel carries (1/s^2, the unit Godot's
  //VehicleWheel3D uses), so heavy and light bodies feel alike. damping_ratio
  //is 1 for a spring that settles with no overshoot
  float stiffness;
  float damping_ratio;

  //the most force the tyre can push sideways or along, as a multiple of the
  //load on it
  float friction_slip;

  //1 applies the sideways force at the contact, 0 at the height of the centre
  //of mass, which is how a body is kept from rolling over in a turn
  float roll_influence;

  bool steering;
  bool traction;

  //what the last step found
  bool grounded;
  float suspension_length;
  float load;

  //how far the spring was pushed past the end of its travel, into the bump
  //stop that holds the body up when it is not enough
  float overtravel;
  float spin_angle;
  float steer_angle;
} PVehicleWheel;

typedef struct PVehicle {
  float mass;
  vec3 inertia;

  PVehicleWheel wheels[PE_VEHICLE_WHEELS_MAX];
  u32 wheel_count;

  //spheres that keep the body out of walls, in the body's axes
  vec3 sphere_centres[PE_VEHICLE_BODY_SPHERES_MAX];
  float sphere_radius;
  u32 sphere_count;

  vec3 gravity;
  float linear_drag;
  float angular_drag;
  float brake_friction;

  //what the driver asks for. steering is an angle in radians, positive to the
  //left, toward +X; engine_force is in newtons shared between the traction wheels,
  //negative to reverse; brake is 0 to 1
  float steering;
  float engine_force;
  float brake;

  vec3 position;
  versor orientation;
  vec3 linear_velocity;
  vec3 angular_velocity;

  //the fastest the body ran into a wall in the last step, in metres a second,
  //0 when it touched none
  float impact_speed;
} PVehicle;

//a box of these half extents (X width, Y length, Z height) decides the inertia
void pe_vehicle_init(PVehicle *vehicle, float mass, const vec3 half_extents);

//false when the body already has PE_VEHICLE_WHEELS_MAX
bool pe_vehicle_add_wheel(PVehicle *vehicle, const PVehicleWheel *wheel);

bool pe_vehicle_add_sphere(PVehicle *vehicle, const vec3 centre);

//puts the body at rest at position, turned about up by heading_degrees
void pe_vehicle_place(PVehicle *vehicle, const vec3 position,
                      float heading_degrees);

//advances by seconds in steps of at most 1/240, against the ground's triangles
void pe_vehicle_step(PVehicle *vehicle, const PCollisionMesh *ground,
                     float seconds);

//the body to world matrix
void pe_vehicle_transform(const PVehicle *vehicle, mat4 out);

//a wheel's own matrix to world, steered, spun and hung at its suspension length
void pe_vehicle_wheel_transform(const PVehicle *vehicle, u32 wheel, mat4 out);

//along the body's forward axis, metres a second, negative in reverse
float pe_vehicle_forward_speed(const PVehicle *vehicle);

#endif // !PE_VEHICLE_H
