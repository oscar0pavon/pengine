#ifndef PE_WOWOBJECT_H
#define PE_WOWOBJECT_H

#include <engine/numbers.h>
#include <stdbool.h>

//ongoing world state, the third phase after wowauth's login and wowworld's
//handshake/char-select: parses SMSG_UPDATE_OBJECT/SMSG_COMPRESSED_UPDATE_
//OBJECT, the packets that carry every creature's spawn position and display
//id, once the player is actually placed in the world, plus SMSG_MONSTER_MOVE,
//which is how the server actually moves a creature once spawned - an
//UPDATETYPE_MOVEMENT block in an UPDATE_OBJECT packet is rare in practice.
//pwow has no character-select-style need for anything but creatures yet, so
//players/items/gameobjects are parsed (to stay in sync) but not kept

#define PE_WOWOBJECT_CREATURES_MAX 512

typedef struct PWowCreature {
  u64 guid;
  u32 entry;
  u32 display_id;
  float x, y, z, o; //what creatures_sync() actually reads - the interpolated
                    //display position/facing, kept current every frame by
                    //pe_wowobject_state_tick() while moving below is set

  //SMSG_MONSTER_MOVE state: a spline from move_from to move_to, timed over
  //move_duration seconds, set by pe_wowobject_handle_monster_move() and
  //advanced by pe_wowobject_state_tick(), which is what actually writes
  //x/y/z/o above between the sparse spline packets the server sends
  bool moving;
  //the last SMSG_MONSTER_MOVE's own spline flags carry a real walk/run bit
  //for classic - PRE_WOTLK_RUNMODE (0x100), set for Run and clear for Walk
  //(WoWee's spline_packet.hpp; WotLK repurposed the same bit as DONE and
  //moved the distinction to separate opcodes instead). meaningless while
  //!moving - the "stop" spline packet leaves this at whatever it last was
  bool walking;
  float move_from_x, move_from_y, move_from_z;
  float move_to_x, move_to_y, move_to_z;
  float move_elapsed, move_duration;
  bool move_has_heading;
  float move_heading;
  bool move_has_final_facing;
  float move_final_facing;
} PWowCreature;

typedef struct PWowObjectState {
  PWowCreature creatures[PE_WOWOBJECT_CREATURES_MAX];
  int count;
} PWowObjectState;

//parses one SMSG_UPDATE_OBJECT (or, with compressed true, SMSG_COMPRESSED_
//UPDATE_OBJECT - decompressed first) payload and folds any creature blocks
//it finds into state: CREATE_OBJECT/CREATE_OBJECT2 add or update a creature
//by guid, MOVEMENT updates the position of one already known,
//OUT_OF_RANGE_OBJECTS removes it. an unrecognised block type (NEAR_OBJECTS,
//or anything future) aborts the rest of *this* packet only; state from
//blocks already applied earlier in it is kept
void pe_wowobject_handle_packet(PWowObjectState *state, const u8 *payload,
                                int payload_len, bool compressed);

//parses one SMSG_MONSTER_MOVE payload and starts (or, for a "stop" packet,
//cancels) the named creature's interpolated move. a guid this has not seen
//a CREATE_OBJECT for yet is ignored, same as everywhere else in this file
void pe_wowobject_handle_monster_move(PWowObjectState *state,
                                      const u8 *payload, int payload_len);

//call once a frame, before creatures_sync() (or anything else) reads
//state->creatures[].x/y/z/o: advances every creature currently mid-spline by
//delta_seconds and writes its interpolated position/facing back into
//x/y/z/o
void pe_wowobject_state_tick(PWowObjectState *state, double delta_seconds);

#endif
