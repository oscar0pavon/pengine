#include "wowworld.h"
#include "wow_wire.h"

#include <openssl/rand.h>

#include <ctype.h>
#include <string.h>

//SMSG_AUTH_CHALLENGE, CMSG_AUTH_SESSION, SMSG_AUTH_RESPONSE - the only
//opcodes a handshake needs. wire values are vanilla's own, confirmed against
//vmangos's own WorldSocket.cpp rather than assumed from a generic table
#define OP_SMSG_AUTH_CHALLENGE 0x1EC
#define OP_CMSG_AUTH_SESSION 0x1ED
#define OP_SMSG_AUTH_RESPONSE 0x1EE

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
