#include "wowobject.h"

#include <zlib.h>

#include <math.h>
#include <string.h>

//block types (Objects/UpdateData.h in vmangos)
#define UPDATETYPE_VALUES 0
#define UPDATETYPE_MOVEMENT 1
#define UPDATETYPE_CREATE_OBJECT 2
#define UPDATETYPE_CREATE_OBJECT2 3
#define UPDATETYPE_OUT_OF_RANGE_OBJECTS 4

#define TYPEID_UNIT 3

#define UPDATEFLAG_TRANSPORT 0x02
#define UPDATEFLAG_MELEE_ATTACKING 0x04
#define UPDATEFLAG_HIGHGUID 0x08
#define UPDATEFLAG_ALL 0x10
#define UPDATEFLAG_LIVING 0x20
#define UPDATEFLAG_HAS_POSITION 0x40

#define MOVEFLAG_JUMPING 0x00002000
#define MOVEFLAG_SWIMMING 0x00200000
#define MOVEFLAG_SPLINE_ENABLED 0x00400000
#define MOVEFLAG_ONTRANSPORT 0x02000000
#define MOVEFLAG_SPLINE_ELEVATION 0x04000000

#define FIELD_OBJECT_ENTRY 3
#define FIELD_UNIT_DISPLAYID 131

//a byte cursor over one already fully-received buffer - not WBuf
//(wow_wire.h), whose 8192-byte storage is sized for one wire packet and too
//small for a decompressed object-update burst (see OBJECT_SCRATCH_MAX
//below). a read past the end clamps to the end and marks overrun rather
//than going out of bounds, so a length miscalculation anywhere above fails
//safe instead of reading garbage
typedef struct Cursor {
  const u8 *data;
  int len;
  int pos;
  bool overrun;
} Cursor;

static u8 cur_u8(Cursor *c) {
  if (c->pos + 1 > c->len) {
    c->overrun = true;
    return 0;
  }
  return c->data[c->pos++];
}

static u32 cur_u32(Cursor *c) {
  if (c->pos + 4 > c->len) {
    c->overrun = true;
    c->pos = c->len;
    return 0;
  }
  u32 v = (u32)(c->data[c->pos] | (c->data[c->pos + 1] << 8) |
               (c->data[c->pos + 2] << 16) | (c->data[c->pos + 3] << 24));
  c->pos += 4;
  return v;
}

static float cur_float(Cursor *c) {
  u32 bits = cur_u32(c);
  float f;
  memcpy(&f, &bits, sizeof(f));
  return f;
}

//a mask byte, then one byte per set bit (LSB first), zero-extended into a
//u64 - not the plain 8-byte guid a MovementInfo's own transport guid or
//wowworld.c's CHAR_ENUM/PLAYER_LOGIN use (ObjectGuid's plain operator<<,
//confirmed in vmangos's ObjectGuid.cpp - only Unit::GetPackGUID() targets,
//like MELEE_ATTACKING's victim below, are actually packed)
static u64 cur_packed_guid(Cursor *c) {
  u8 mask = cur_u8(c);
  u64 guid = 0;
  for (int i = 0; i < 8; i++)
    if (mask & (1 << i))
      guid |= ((u64)cur_u8(c)) << (i * 8);
  return guid;
}

static u64 cur_u64(Cursor *c) {
  u64 lo = cur_u32(c);
  u64 hi = cur_u32(c);
  return lo | (hi << 32);
}

//MoveSplineFlag (Movement/spline/MoveSplineFlag.h)
#define SPLINEFLAG_FINAL_POINT 0x00010000
#define SPLINEFLAG_FINAL_TARGET 0x00020000
#define SPLINEFLAG_FINAL_ANGLE 0x00040000

//generous but bounded, so a corrupt nodes count can't spin forever - a real
//spline path is at most a few dozen points
#define SPLINE_NODES_MAX 4096

//Movement::PacketBuilder::WriteCreate (Movement/spline/packet_builder.cpp),
//appended in place by Object::BuildMovementUpdate right after the speeds
//whenever MOVEFLAG_SPLINE_ENABLED is set - not a separately-framed
//sub-packet, just more of the same movement block
static bool parse_movement_spline(Cursor *c) {
  u32 spline_flags = cur_u32(c);

  if (spline_flags & SPLINEFLAG_FINAL_ANGLE)
    cur_float(c);
  else if (spline_flags & SPLINEFLAG_FINAL_TARGET)
    cur_u64(c); //plain guid (FacingInfo::target), not packed
  else if (spline_flags & SPLINEFLAG_FINAL_POINT) {
    cur_float(c);
    cur_float(c);
    cur_float(c);
  }

  cur_u32(c); //time passed
  cur_u32(c); //duration
  cur_u32(c); //spline id (build 5875 > CLIENT_BUILD_1_7_1, always present)

  u32 nodes = cur_u32(c);
  if (nodes > SPLINE_NODES_MAX)
    return false;
  for (u32 i = 0; i < nodes; i++) {
    cur_float(c);
    cur_float(c);
    cur_float(c);
  }

  cur_float(c); //final destination x,y,z (Vector3::zero() when cyclic, but
  cur_float(c); //always written either way)
  cur_float(c);

  return true;
}

//true if fully consumed (safe to keep reading the packet). x/y/z/o and
//*has_position are always valid on return - position is read before
//anything that can fail
static bool parse_movement_block(Cursor *c, bool *has_position, float *x,
                                 float *y, float *z, float *o) {
  u8 update_flags = cur_u8(c);
  *has_position = false;

  if (update_flags & UPDATEFLAG_LIVING) {
    u32 move_flags = cur_u32(c);
    cur_u32(c); //timestamp

    *x = cur_float(c);
    *y = cur_float(c);
    *z = cur_float(c);
    *o = cur_float(c);
    *has_position = true;

    if (move_flags & MOVEFLAG_ONTRANSPORT) {
      cur_u64(c); //plain guid (MovementInfo::t_guid), not packed
      cur_float(c);
      cur_float(c);
      cur_float(c);
      cur_float(c);
    }
    if (move_flags & MOVEFLAG_SWIMMING)
      cur_float(c); //pitch
    cur_u32(c);      //fall time
    if (move_flags & MOVEFLAG_JUMPING) {
      cur_float(c);
      cur_float(c);
      cur_float(c);
      cur_float(c);
    }
    if (move_flags & MOVEFLAG_SPLINE_ELEVATION)
      cur_float(c);
    for (int i = 0; i < 6; i++)
      cur_float(c); //walk/run/runback/swim/swimback/turn speeds

    if (move_flags & MOVEFLAG_SPLINE_ENABLED && !parse_movement_spline(c))
      return false;
  } else if (update_flags & UPDATEFLAG_HAS_POSITION) {
    *x = cur_float(c);
    *y = cur_float(c);
    *z = cur_float(c);
    *o = cur_float(c);
    *has_position = true;
  }

  if (update_flags & UPDATEFLAG_HIGHGUID)
    cur_u32(c);
  if (update_flags & UPDATEFLAG_ALL)
    cur_u32(c);
  if (update_flags & UPDATEFLAG_MELEE_ATTACKING)
    cur_packed_guid(c);
  if (update_flags & UPDATEFLAG_TRANSPORT)
    cur_u32(c);

  return true;
}

//generously above any real 1.12 object's field count (a creature's UNIT_END
//is 188 fields, 6 mask blocks) - if a blockCount off the wire ever exceeds
//this, something is already wrong and the block is refused rather than
//read into an undersized array
#define VALUES_MASK_BYTES_MAX 128

static bool parse_values_block(Cursor *c, u32 *entry, u32 *display_id) {
  u8 block_count = cur_u8(c);
  int mask_bytes = block_count * 4;
  if (mask_bytes > VALUES_MASK_BYTES_MAX)
    return false;

  u8 mask[VALUES_MASK_BYTES_MAX];
  for (int i = 0; i < mask_bytes; i++)
    mask[i] = cur_u8(c);

  int field = 0;
  for (int byte_i = 0; byte_i < mask_bytes; byte_i++) {
    for (int bit = 0; bit < 8; bit++, field++) {
      if (!(mask[byte_i] & (1 << bit)))
        continue;
      u32 value = cur_u32(c);
      if (field == FIELD_OBJECT_ENTRY)
        *entry = value;
      else if (field == FIELD_UNIT_DISPLAYID)
        *display_id = value;
    }
  }
  return true;
}

static PWowCreature *find_creature(PWowObjectState *state, u64 guid) {
  for (int i = 0; i < state->count; i++)
    if (state->creatures[i].guid == guid)
      return &state->creatures[i];
  return NULL;
}

static PWowCreature *find_or_add_creature(PWowObjectState *state, u64 guid) {
  PWowCreature *existing = find_creature(state, guid);
  if (existing)
    return existing;
  if (state->count >= PE_WOWOBJECT_CREATURES_MAX)
    return NULL;
  PWowCreature *c = &state->creatures[state->count++];
  memset(c, 0, sizeof(*c));
  c->guid = guid;
  return c;
}

static void remove_creature(PWowObjectState *state, u64 guid) {
  for (int i = 0; i < state->count; i++) {
    if (state->creatures[i].guid == guid) {
      state->creatures[i] = state->creatures[state->count - 1];
      state->count--;
      return;
    }
  }
}

//single-threaded like the rest of pengine's update/draw (see this repo's
//CLAUDE.md) - one scratch buffer is fine. generous for one starting zone's
//worth of nearby creatures in a single compressed burst
#define OBJECT_SCRATCH_MAX 65536
static u8 object_scratch[OBJECT_SCRATCH_MAX];

void pe_wowobject_handle_packet(PWowObjectState *state, const u8 *payload,
                                int payload_len, bool compressed) {
  const u8 *data = payload;
  int len = payload_len;

  if (compressed) {
    if (payload_len < 4)
      return;
    u32 uncompressed_size;
    memcpy(&uncompressed_size, payload, 4); //native-endian, both ends x86
    if (uncompressed_size > OBJECT_SCRATCH_MAX)
      return;
    uLongf dest_len = uncompressed_size;
    if (uncompress(object_scratch, &dest_len, payload + 4,
                   (uLong)(payload_len - 4)) != Z_OK)
      return;
    data = object_scratch;
    len = (int)dest_len;
  }

  Cursor c = {data, len, 0, false};
  u32 block_count = cur_u32(&c);
  cur_u8(&c); //hasTransport

  for (u32 i = 0; i < block_count && !c.overrun; i++) {
    u8 update_type = cur_u8(&c);

    if (update_type == UPDATETYPE_OUT_OF_RANGE_OBJECTS) {
      u32 n = cur_u32(&c);
      for (u32 j = 0; j < n && !c.overrun; j++)
        remove_creature(state, cur_packed_guid(&c));
      continue;
    }

    //anything else (NEAR_OBJECTS, or a future block type) has a layout not
    //decoded here - abort the rest of this packet rather than misparse it
    if (update_type != UPDATETYPE_VALUES && update_type != UPDATETYPE_MOVEMENT &&
        update_type != UPDATETYPE_CREATE_OBJECT &&
        update_type != UPDATETYPE_CREATE_OBJECT2)
      return;

    u64 guid = cur_packed_guid(&c);

    if (update_type == UPDATETYPE_VALUES) {
      u32 entry = 0, display_id = 0;
      if (!parse_values_block(&c, &entry, &display_id))
        return;
      PWowCreature *existing = find_creature(state, guid);
      if (existing) {
        if (entry)
          existing->entry = entry;
        if (display_id)
          existing->display_id = display_id;
      }
      continue;
    }

    if (update_type == UPDATETYPE_MOVEMENT) {
      bool has_position;
      float x, y, z, o;
      bool ok = parse_movement_block(&c, &has_position, &x, &y, &z, &o);
      if (has_position) {
        PWowCreature *existing = find_creature(state, guid);
        if (existing) {
          //game axes are X north, Y west; pwow's world (and player_position,
          //which anything storing a PWowCreature will compare this against)
          //is X north, Y east - same flip pe_terrain_point_y and live_login()
          //already apply, done here at the one point a wire position enters
          //the registry rather than by every caller
          existing->x = x;
          existing->y = -y;
          existing->z = z;
          existing->o = o;
        }
      }
      if (!ok)
        return;
      continue;
    }

    //CREATE_OBJECT / CREATE_OBJECT2
    u8 object_type = cur_u8(&c);
    bool has_position;
    float x = 0, y = 0, z = 0, o = 0;
    bool movement_ok = parse_movement_block(&c, &has_position, &x, &y, &z, &o);
    u32 entry = 0, display_id = 0;
    bool values_ok = movement_ok && parse_values_block(&c, &entry, &display_id);

    if (movement_ok && values_ok && object_type == TYPEID_UNIT) {
      PWowCreature *creature = find_or_add_creature(state, guid);
      if (creature) {
        if (entry)
          creature->entry = entry;
        if (display_id)
          creature->display_id = display_id;
        if (has_position) {
          creature->x = x;
          creature->y = -y; //see the same flip's comment in the MOVEMENT case
          creature->z = z;
          creature->o = o;
        }
      }
    }

    if (!movement_ok || !values_ok)
      return;
  }
}

//---------------------------------------------------------------------------
//SMSG_MONSTER_MOVE (Movement::PacketBuilder::WriteMonsterMove /
//WriteCommonMonsterMovePart, packet_builder.cpp) - a different wire layout
//from the spline embedded in an UPDATE_OBJECT movement block
//(parse_movement_spline above, PacketBuilder::WriteCreate): this one is its
//own opcode, sent whenever a creature's path actually changes, and is what
//makes a spawned creature walk rather than stand still
//---------------------------------------------------------------------------

//MonsterMoveType (packet_builder.cpp) - the byte WriteCommonMonsterMovePart
//writes in place of a facing angle/target/spot when there is none. value 1
//(Stop) is never written by that path; it only appears in the short "stop"
//packet below, which is how the two are told apart
#define MONSTER_MOVE_FACING_SPOT 2
#define MONSTER_MOVE_FACING_TARGET 3
#define MONSTER_MOVE_FACING_ANGLE 4

//MoveSplineFlag::Flying / ::Cyclic (MoveSplineFlag.h) - the two bits of the
//flags word this cares about; the rest (Mask_No_Monster_Move) never reach
//the wire in this packet, already stripped server side
#define MOVE_SPLINEFLAG_CATMULLROM 0x00000200
#define MOVE_SPLINEFLAG_CYCLIC 0x00100000
//SplineFlag::PRE_WOTLK_RUNMODE (spline_packet.hpp) - set means Run, clear
//means Walk, for the pre-WotLK wire this server speaks
#define MOVE_SPLINEFLAG_RUNMODE 0x00000100

void pe_wowobject_handle_monster_move(PWowObjectState *state,
                                      const u8 *payload, int payload_len) {
  Cursor c = {payload, payload_len, 0, false};
  u64 guid = cur_packed_guid(&c);
  float start_x = cur_float(&c);
  float start_y = cur_float(&c);
  float start_z = cur_float(&c);
  cur_u32(&c); //spline id, not tracked

  if (c.overrun)
    return;

  PWowCreature *creature = find_creature(state, guid);
  if (!creature)
    return;

  //Unit::UpdateSplineMovement's "done" case (MoveSplineInit.cpp) writes a
  //short packet instead - guid, position, spline id, then just a bare stop
  //marker - rather than going through WriteCommonMonsterMovePart at all.
  //that leaves exactly one byte after the fields already read above, versus
  //the many more that follow in every other case, which is what tells the
  //two formats apart here
  if (c.len - c.pos == 1) {
    creature->moving = false;
    creature->x = start_x;
    creature->y = -start_y; //see pe_wowobject_handle_packet's MOVEMENT case
    creature->z = start_z;
    return;
  }

  u8 facing_type = cur_u8(&c);
  bool has_final_facing = false;
  float final_facing = 0;
  if (facing_type == MONSTER_MOVE_FACING_ANGLE) {
    final_facing = cur_float(&c);
    has_final_facing = true;
  } else if (facing_type == MONSTER_MOVE_FACING_TARGET) {
    cur_u64(&c); //a guid to face - not tracked, falls back to travel heading
  } else if (facing_type == MONSTER_MOVE_FACING_SPOT) {
    cur_float(&c); //likewise a fixed point to face - not tracked
    cur_float(&c);
    cur_float(&c);
  }

  u32 spline_flags = cur_u32(&c);
  u32 duration_ms = cur_u32(&c);

  float dest_x = 0, dest_y = 0, dest_z = 0;
  if (spline_flags & MOVE_SPLINEFLAG_CATMULLROM) {
    u32 nodes = cur_u32(&c);
    if (nodes == 0 || nodes > SPLINE_NODES_MAX)
      return;
    for (u32 i = 0; i < nodes; i++) {
      float x = cur_float(&c), y = cur_float(&c), z = cur_float(&c);
      if (i == nodes - 1) {
        dest_x = x;
        dest_y = y;
        dest_z = z;
      }
    }
    //a cyclic path (a patrol loop) has no single endpoint to head for -
    //left unimplemented, the creature holds its last position until the
    //server sends it a non-cyclic move instead
    if (spline_flags & MOVE_SPLINEFLAG_CYCLIC)
      return;
  } else {
    u32 count = cur_u32(&c);
    if (count == 0)
      return;
    dest_x = cur_float(&c);
    dest_y = cur_float(&c);
    dest_z = cur_float(&c);
    //the remaining count-1 words are ByteBuffer::appendPackXYZ-packed
    //intermediate waypoints describing the curve's shape between here and
    //the destination - skipped; pwow interpolates straight to the
    //destination rather than tracing the path the server drew
  }

  if (c.overrun)
    return;

  creature->moving = true;
  creature->walking = (spline_flags & MOVE_SPLINEFLAG_RUNMODE) == 0;
  creature->move_from_x = start_x;
  creature->move_from_y = -start_y;
  creature->move_from_z = start_z;
  creature->move_to_x = dest_x;
  creature->move_to_y = -dest_y;
  creature->move_to_z = dest_z;
  creature->move_elapsed = 0.0f;
  creature->move_duration = (float)duration_ms / 1000.0f;
  creature->move_has_final_facing = has_final_facing;
  creature->move_final_facing = final_facing;

  //face the direction of travel by default, the same fallback the game's
  //own client applies absent an explicit facing override. o is x-north,
  //y-west and right-handed like everything else read off the wire (this
  //repo's CLAUDE.md), so the heading is a plain atan2 over the un-flipped
  //deltas, stored unflipped like every other wire o value in this file
  creature->move_has_heading = start_x != dest_x || start_y != dest_y;
  if (creature->move_has_heading)
    creature->move_heading = atan2f(dest_y - start_y, dest_x - start_x);
}

void pe_wowobject_state_tick(PWowObjectState *state, double delta_seconds) {
  for (int i = 0; i < state->count; i++) {
    PWowCreature *creature = &state->creatures[i];
    if (!creature->moving)
      continue;

    creature->move_elapsed += (float)delta_seconds;
    float t = creature->move_duration > 0.0f
                  ? creature->move_elapsed / creature->move_duration
                  : 1.0f;
    bool arrived = t >= 1.0f;
    if (arrived)
      t = 1.0f;

    creature->x =
        creature->move_from_x + (creature->move_to_x - creature->move_from_x) * t;
    creature->y =
        creature->move_from_y + (creature->move_to_y - creature->move_from_y) * t;
    creature->z =
        creature->move_from_z + (creature->move_to_z - creature->move_from_z) * t;

    if (creature->move_has_heading)
      creature->o = creature->move_heading;

    if (arrived) {
      creature->moving = false;
      if (creature->move_has_final_facing)
        creature->o = creature->move_final_facing;
    }
  }
}
