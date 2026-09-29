#include "wowworld.h"
#include "wow_wire.h"

#include <openssl/rand.h>

#include <ctype.h>
#include <string.h>
#include <sys/select.h>

//opcode numbers are vanilla 1.12.1's own (Opcodes_1_12_1.h), confirmed
//against vmangos's own source rather than assumed from a generic table
#define OP_SMSG_AUTH_CHALLENGE 0x1EC
#define OP_CMSG_AUTH_SESSION 0x1ED
#define OP_SMSG_AUTH_RESPONSE 0x1EE
#define OP_CMSG_CHAR_ENUM 55
#define OP_SMSG_CHAR_ENUM 59
#define OP_CMSG_PLAYER_LOGIN 61
#define OP_SMSG_LOGIN_VERIFY_WORLD 566
#define OP_SMSG_UPDATE_OBJECT 169
#define OP_SMSG_COMPRESSED_UPDATE_OBJECT 502
#define OP_SMSG_MONSTER_MOVE 221

//SharedDefines.h's ResponseCodes enum, position 12 (RESPONSE_SUCCESS..
//CSTATUS_AUTHENTICATING fill 0..11 first)
#define AUTH_OK 12

static void fail(char *error, int error_max, const char *msg) {
  if (error && error_max > 0)
    snprintf(error, error_max, "%s", msg);
}

//---------------------------------------------------------------------------
//the vanilla (build <= 5875) header cipher: a XOR against a repeating key,
//chained by adding the previous *output* byte in. real RC4 and the
//CMaNGOS-TBC HMAC-derived key variant are different builds' problem, not
//this one's
//---------------------------------------------------------------------------

static void cipher_encrypt(PWowWorld *w, u8 *data, int len) {
  for (int i = 0; i < len; i++) {
    u8 x = (u8)((data[i] ^ w->cipher_key[w->send_index]) + w->send_prev);
    w->send_index = (u8)((w->send_index + 1) % w->cipher_key_len);
    data[i] = x;
    w->send_prev = x;
  }
}

static void cipher_decrypt(PWowWorld *w, u8 *data, int len) {
  for (int i = 0; i < len; i++) {
    u8 enc = data[i];
    u8 x = (u8)((enc - w->recv_prev) ^ w->cipher_key[w->recv_index]);
    w->recv_index = (u8)((w->recv_index + 1) % w->cipher_key_len);
    w->recv_prev = enc;
    data[i] = x;
  }
}

bool pe_wowworld_send_packet(PWowWorld *world, u16 opcode, const u8 *payload,
                             int payload_len) {
  //CMSG header: size (2 bytes, big-endian, counts the opcode's 4 bytes but
  //not itself) + opcode (4 bytes, little-endian; top 2 always zero, every
  //CMSG opcode fits in 16 bits)
  u16 size_field = (u16)(payload_len + 4);
  u8 header[6];
  header[0] = (u8)((size_field >> 8) & 0xFF);
  header[1] = (u8)(size_field & 0xFF);
  header[2] = (u8)(opcode & 0xFF);
  header[3] = (u8)((opcode >> 8) & 0xFF);
  header[4] = 0;
  header[5] = 0;

  if (world->cipher_key_len > 0)
    cipher_encrypt(world, header, 6);

  if (!send_all(world->fd, header, 6))
    return false;
  if (payload_len > 0 && !send_all(world->fd, payload, payload_len))
    return false;
  return true;
}

bool pe_wowworld_read_packet(PWowWorld *world, u16 *opcode, u8 *payload,
                             int payload_max, int *payload_len) {
  //SMSG header: size (2 bytes, big-endian, counts the opcode's 2 bytes but
  //not itself) + opcode (2 bytes, little-endian). Server-to-client opcodes
  //are half the width of client-to-server ones - a real protocol asymmetry,
  //not a typo
  u8 header[4];
  if (!read_exact(world->fd, header, 4))
    return false;

  if (world->cipher_key_len > 0)
    cipher_decrypt(world, header, 4);

  u16 size = (u16)((header[0] << 8) | header[1]);
  u16 op = (u16)(header[2] | (header[3] << 8));
  if (size < 2)
    return false;
  int len = size - 2;
  if (len > payload_max)
    return false;

  if (len > 0 && !read_exact(world->fd, payload, len))
    return false;

  *opcode = op;
  *payload_len = len;
  return true;
}

void pe_wowworld_close(PWowWorld *world) {
  if (world->connected)
    close(world->fd);
  world->connected = false;
}

bool pe_wowworld_connect(const char *host, int port, const char *account,
                         const u8 *session_key, u32 build, u32 realm_id,
                         PWowWorld *world, char *error, int error_max) {
  memset(world, 0, sizeof(*world));

  world->fd = tcp_connect(host, port);
  if (world->fd < 0) {
    fail(error, error_max, "could not connect to mangosd");
    return false;
  }
  world->connected = true;

  //SMSG_AUTH_CHALLENGE: a bare 4-byte server seed, sent before any
  //encryption is set up on either side
  u16 opcode;
  u8 payload[64];
  int payload_len;
  if (!pe_wowworld_read_packet(world, &opcode, payload, sizeof(payload),
                               &payload_len) ||
      opcode != OP_SMSG_AUTH_CHALLENGE || payload_len < 4) {
    fail(error, error_max, "malformed SMSG_AUTH_CHALLENGE");
    pe_wowworld_close(world);
    return false;
  }
  u32 server_seed = (u32)(payload[0] | (payload[1] << 8) | (payload[2] << 16) |
                          (payload[3] << 24));

  u32 client_seed;
  RAND_bytes((u8 *)&client_seed, sizeof(client_seed));

  char upper_account[64];
  snprintf(upper_account, sizeof(upper_account), "%s", account);
  for (char *p = upper_account; *p; p++)
    *p = (char)toupper((unsigned char)*p);

  //digest = SHA1(account | 0,0,0,0 | clientSeed | serverSeed | sessionKey),
  //matching vmangos's own WorldSocket::_HandleAuthSession byte for byte -
  //that is the account it checks against, not a generic client's guess at it
  u8 digest_input[64 + 4 + 4 + 4 + 40];
  int p = 0;
  int account_len = (int)strlen(upper_account);
  memcpy(digest_input + p, upper_account, account_len);
  p += account_len;
  memset(digest_input + p, 0, 4);
  p += 4;
  memcpy(digest_input + p, &client_seed, 4);
  p += 4;
  memcpy(digest_input + p, &server_seed, 4);
  p += 4;
  memcpy(digest_input + p, session_key, 40);
  p += 40;
  u8 digest[20];
  ww_sha1(digest_input, p, digest);

  //CMSG_AUTH_SESSION, vanilla layout: build, realm id, account, client seed,
  //digest. vmangos's own addon-info reader treats a missing or empty addon
  //block as "no addons" rather than a protocol error (AddonHandler::
  //BuildAddonPacket bails out to false without touching the auth result), so
  //it is left off entirely rather than sending a zlib stream nothing reads
  WBuf out;
  out.len = 0;
  wbuf_u32(&out, build);
  wbuf_u32(&out, realm_id);
  wbuf_cstring(&out, upper_account);
  wbuf_u32(&out, client_seed);
  wbuf_bytes(&out, digest, 20);

  if (!pe_wowworld_send_packet(world, OP_CMSG_AUTH_SESSION, out.data,
                               out.len)) {
    fail(error, error_max, "failed sending CMSG_AUTH_SESSION");
    pe_wowworld_close(world);
    return false;
  }

  //the client (and, symmetrically, the server) turns encryption on the
  //instant AUTH_SESSION is sent - SMSG_AUTH_RESPONSE itself already comes
  //back through the cipher
  memcpy(world->cipher_key, session_key, 40);
  world->cipher_key_len = 40;

  if (!pe_wowworld_read_packet(world, &opcode, payload, sizeof(payload),
                               &payload_len) ||
      opcode != OP_SMSG_AUTH_RESPONSE || payload_len < 1) {
    fail(error, error_max, "malformed SMSG_AUTH_RESPONSE");
    pe_wowworld_close(world);
    return false;
  }
  if (payload[0] != AUTH_OK) {
    fail(error, error_max, "world server rejected the session");
    pe_wowworld_close(world);
    return false;
  }

  return true;
}

//how many unrelated packets to read past before giving up on finding a
//specific one. generous: a crowded login can queue a lot of state
//(reputation, action bars, the player's own object update...) ahead of the
//packet actually being waited for
#define WAIT_FOR_OPCODE_ATTEMPTS 256

bool pe_wowworld_char_enum(PWowWorld *world, PWowCharacter *out, int out_max,
                           int *count, char *error, int error_max) {
  if (!pe_wowworld_send_packet(world, OP_CMSG_CHAR_ENUM, NULL, 0)) {
    fail(error, error_max, "failed sending CMSG_CHAR_ENUM");
    return false;
  }

  u8 payload[PE_WOWWORLD_PACKET_MAX];
  u16 opcode;
  int payload_len = 0;
  bool found = false;
  for (int attempt = 0; attempt < WAIT_FOR_OPCODE_ATTEMPTS; attempt++) {
    if (!pe_wowworld_read_packet(world, &opcode, payload, sizeof(payload),
                                 &payload_len)) {
      fail(error, error_max, "disconnected while waiting for SMSG_CHAR_ENUM");
      return false;
    }
    if (opcode == OP_SMSG_CHAR_ENUM) {
      found = true;
      break;
    }
  }
  if (!found) {
    fail(error, error_max, "never saw SMSG_CHAR_ENUM");
    return false;
  }

  WBuf buf;
  memcpy(buf.data, payload, payload_len);
  buf.len = payload_len;
  buf.pos = 0;

  u8 num = wbuf_read_u8(&buf);
  *count = 0;
  for (int i = 0; i < num && *count < out_max; i++) {
    PWowCharacter *c = &out[*count];
    c->guid = wbuf_read_u64(&buf);
    wbuf_read_string(&buf, c->name, sizeof(c->name));

    //the rest of Player::BuildEnumData's entry, none of which pwow needs
    //yet: race, class, gender, skin, face, hair style, hair color, facial
    //hair (8 x u8); level (u8); zone, map (2 x u32); x, y, z (3 x float);
    //guild id, character flags (2 x u32); first-login flag (u8); pet
    //display id, level, family (3 x u32); 20 equipment slots, each a
    //display id (u32) plus an inventory type (u8)
    buf.pos += 8;
    buf.pos += 1;
    buf.pos += 4 + 4;
    buf.pos += 4 + 4 + 4;
    buf.pos += 4 + 4;
    buf.pos += 1;
    buf.pos += 4 + 4 + 4;
    buf.pos += 20 * (4 + 1);

    (*count)++;
  }

  return true;
}

bool pe_wowworld_player_login(PWowWorld *world, u64 guid,
                              PWowLoginResult *out, char *error,
                              int error_max) {
  WBuf req;
  req.len = 0;
  wbuf_u64(&req, guid);

  if (!pe_wowworld_send_packet(world, OP_CMSG_PLAYER_LOGIN, req.data,
                               req.len)) {
    fail(error, error_max, "failed sending CMSG_PLAYER_LOGIN");
    return false;
  }

  u8 payload[PE_WOWWORLD_PACKET_MAX];
  u16 opcode;
  int payload_len = 0;
  bool found = false;
  for (int attempt = 0; attempt < WAIT_FOR_OPCODE_ATTEMPTS; attempt++) {
    if (!pe_wowworld_read_packet(world, &opcode, payload, sizeof(payload),
                                 &payload_len)) {
      fail(error, error_max,
          "disconnected while waiting for SMSG_LOGIN_VERIFY_WORLD");
      return false;
    }
    if (opcode == OP_SMSG_LOGIN_VERIFY_WORLD) {
      found = true;
      break;
    }
  }
  if (!found) {
    fail(error, error_max, "never saw SMSG_LOGIN_VERIFY_WORLD");
    return false;
  }

  WBuf buf;
  memcpy(buf.data, payload, payload_len);
  buf.len = payload_len;
  buf.pos = 0;
  out->map = wbuf_read_u32(&buf);
  out->x = wbuf_read_float(&buf);
  out->y = wbuf_read_float(&buf);
  out->z = wbuf_read_float(&buf);
  out->o = wbuf_read_float(&buf);

  return true;
}

//zero-timeout select(): true only if a full read would not block right now
static bool data_ready(PWowWorld *world) {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(world->fd, &fds);
  struct timeval tv = {0, 0};
  return select(world->fd + 1, &fds, NULL, NULL, &tv) > 0;
}

#define POLL_PACKETS_MAX 64

void pe_wowworld_poll(PWowWorld *world, PWowObjectState *state) {
  u8 payload[PE_WOWWORLD_PACKET_MAX];
  for (int i = 0; i < POLL_PACKETS_MAX && world->connected; i++) {
    if (!data_ready(world))
      return;

    u16 opcode;
    int payload_len;
    if (!pe_wowworld_read_packet(world, &opcode, payload, sizeof(payload),
                                 &payload_len)) {
      world->connected = false;
      return;
    }
    if (opcode == OP_SMSG_UPDATE_OBJECT)
      pe_wowobject_handle_packet(state, payload, payload_len, false);
    else if (opcode == OP_SMSG_COMPRESSED_UPDATE_OBJECT)
      pe_wowobject_handle_packet(state, payload, payload_len, true);
    else if (opcode == OP_SMSG_MONSTER_MOVE)
      pe_wowobject_handle_monster_move(state, payload, payload_len);
  }
}
