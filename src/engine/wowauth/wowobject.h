#ifndef PE_WOWOBJECT_H
#define PE_WOWOBJECT_H

#include <engine/numbers.h>
#include <stdbool.h>

//ongoing world state, the third phase after wowauth's login and wowworld's
//handshake/char-select: parses SMSG_UPDATE_OBJECT/SMSG_COMPRESSED_UPDATE_
//OBJECT, the packets that carry every creature's spawn position and display
//id, once the player is actually placed in the world. pwow has no character-
//select-style need for anything but creatures yet, so players/items/
//gameobjects are parsed (to stay in sync) but not kept

#define PE_WOWOBJECT_CREATURES_MAX 512

typedef struct PWowCreature {
  u64 guid;
  u32 entry;
  u32 display_id;
  float x, y, z, o;
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

#endif
