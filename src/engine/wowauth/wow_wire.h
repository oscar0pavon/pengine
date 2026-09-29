#ifndef PE_WOW_WIRE_H
#define PE_WOW_WIRE_H

//private helpers shared by wowauth.c (realmd) and wowworld.c (mangosd): the
//flat wire buffer both protocols frame packets into, and the blocking
//sockets both connect with. not a public engine api, just plumbing one
//login flow's two phases would otherwise duplicate

#include <engine/numbers.h>
#include <stdbool.h>

#include <openssl/sha.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void ww_reverse_bytes(u8 *buf, int len) {
  for (int i = 0; i < len / 2; i++) {
    u8 tmp = buf[i];
    buf[i] = buf[len - 1 - i];
    buf[len - 1 - i] = tmp;
  }
}

static void ww_sha1(const u8 *data, int len, u8 out[20]) { SHA1(data, len, out); }

//---------------------------------------------------------------------------
//wire buffer: a flat byte array plus independent write/read cursors. sized
//to match PE_WOWWORLD_PACKET_MAX (wowworld.h) so a full packet payload
//always fits when copied in for parsing - a mismatch here is a silent
//buffer overflow, not a compile error
//---------------------------------------------------------------------------

typedef struct WBuf {
  u8 data[8192];
  int len;
  int pos;
} WBuf;

static void wbuf_u8(WBuf *b, u8 v) { b->data[b->len++] = v; }
static void wbuf_u16(WBuf *b, u16 v) {
  wbuf_u8(b, (u8)(v & 0xFF));
  wbuf_u8(b, (u8)((v >> 8) & 0xFF));
}
static void wbuf_u32(WBuf *b, u32 v) {
  wbuf_u8(b, (u8)(v & 0xFF));
  wbuf_u8(b, (u8)((v >> 8) & 0xFF));
  wbuf_u8(b, (u8)((v >> 16) & 0xFF));
  wbuf_u8(b, (u8)((v >> 24) & 0xFF));
}
static void wbuf_bytes(WBuf *b, const u8 *data, int n) {
  memcpy(b->data + b->len, data, n);
  b->len += n;
}
static void wbuf_cstring(WBuf *b, const char *s) {
  int n = (int)strlen(s) + 1; //include the null terminator
  memcpy(b->data + b->len, s, n);
  b->len += n;
}
static void wbuf_u64(WBuf *b, u64 v) {
  wbuf_u32(b, (u32)(v & 0xFFFFFFFFu));
  wbuf_u32(b, (u32)(v >> 32));
}

static u8 wbuf_read_u8(WBuf *b) { return b->data[b->pos++]; }
static u16 wbuf_read_u16(WBuf *b) {
  u16 v = (u16)(b->data[b->pos] | (b->data[b->pos + 1] << 8));
  b->pos += 2;
  return v;
}
static u32 wbuf_read_u32(WBuf *b) {
  u32 v = (u32)(b->data[b->pos] | (b->data[b->pos + 1] << 8) |
                (b->data[b->pos + 2] << 16) | (b->data[b->pos + 3] << 24));
  b->pos += 4;
  return v;
}
//ObjectGuid on the wire is a plain 8-byte little-endian value - packed/
//compressed guids are a different, bit-masked encoding used only inside
//object-update fields, not here
static u64 wbuf_read_u64(WBuf *b) {
  u64 lo = wbuf_read_u32(b);
  u64 hi = wbuf_read_u32(b);
  return lo | (hi << 32);
}
static float wbuf_read_float(WBuf *b) {
  u32 bits = wbuf_read_u32(b);
  float f;
  memcpy(&f, &bits, sizeof(f));
  return f;
}
static void wbuf_read_string(WBuf *b, char *out, int max) {
  int i = 0;
  while (b->data[b->pos] != 0 && i < max - 1)
    out[i++] = (char)b->data[b->pos++];
  out[i] = 0;
  b->pos++; //the null terminator itself
}

//---------------------------------------------------------------------------
//blocking tcp
//---------------------------------------------------------------------------

static int tcp_connect(const char *host, int port) {
  char portstr[8];
  snprintf(portstr, sizeof(portstr), "%d", port);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *res;
  if (getaddrinfo(host, portstr, &hints, &res) != 0)
    return -1;

  int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd >= 0 && connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    close(fd);
    fd = -1;
  }
  freeaddrinfo(res);
  return fd;
}

static bool send_all(int fd, const u8 *buf, int n) {
  int sent = 0;
  while (sent < n) {
    ssize_t s = send(fd, buf + sent, (size_t)(n - sent), 0);
    if (s <= 0)
      return false;
    sent += (int)s;
  }
  return true;
}

static bool read_exact(int fd, u8 *buf, int n) {
  int got = 0;
  while (got < n) {
    ssize_t r = recv(fd, buf + got, (size_t)(n - got), 0);
    if (r <= 0)
      return false;
    got += (int)r;
  }
  return true;
}

#endif
