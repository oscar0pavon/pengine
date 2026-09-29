#ifndef PE_WOWWORLD_H
#define PE_WOWWORLD_H

#include <engine/numbers.h>
#include <stdbool.h>

#define PE_WOWWORLD_ERROR_MAX 128
#define PE_WOWWORLD_PACKET_MAX 8192

//a live connection to a world server (mangosd), past the CMSG_AUTH_SESSION
//handshake: every packet from here on has its header run through the
//vanilla XOR+add chaining cipher (build <= 5875; TBC/WotLK use different
//keys or real RC4, which this does not implement)
typedef struct PWowWorld {
  int fd;
  bool connected;

  u8 cipher_key[40];
  int cipher_key_len;
  u8 send_index, send_prev;
  u8 recv_index, recv_prev;
} PWowWorld;

//connects to host:port and completes the CMSG_AUTH_SESSION handshake using
//the session key pe_wowauth_login already returned, leaving the header
//cipher initialized for every packet after. blocking: does not return until
//the handshake finishes or fails. on success, world->connected is true and
//the connection is ready for pe_wowworld_send/read_packet
bool pe_wowworld_connect(const char *host, int port, const char *account,
                         const u8 *session_key, u32 build, u32 realm_id,
                         PWowWorld *world, char *error, int error_max);

//sends one packet, encrypting its header first if the handshake has run
bool pe_wowworld_send_packet(PWowWorld *world, u16 opcode, const u8 *payload,
                             int payload_len);

//blocks until one full packet arrives, decrypting its header first. false on
//disconnect or a payload too big for payload_max
bool pe_wowworld_read_packet(PWowWorld *world, u16 *opcode, u8 *payload,
                             int payload_max, int *payload_len);

void pe_wowworld_close(PWowWorld *world);

#endif
