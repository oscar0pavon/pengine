#ifndef PE_WOWAUTH_H
#define PE_WOWAUTH_H

#include <engine/numbers.h>
#include <stdbool.h>

#define PE_WOWAUTH_REALMS_MAX 32
#define PE_WOWAUTH_ERROR_MAX 128

//one entry of the realm list a logon server answers with, vanilla (1.12)
//layout: a u32 icon and no lock byte, unlike TBC/WotLK's u8 icon+lock
typedef struct PWowRealm {
  char name[64];
  char address[64]; //"ip:port", exactly as the server sent it
  u8 icon;
  u8 flags;
  float population;
  u8 id;
} PWowRealm;

typedef struct PWowAuthResult {
  bool success;
  char error[PE_WOWAUTH_ERROR_MAX];

  //the SRP6 interleaved session key (K), needed to open a world session once
  //one is
  u8 session_key[40];

  PWowRealm realms[PE_WOWAUTH_REALMS_MAX];
  int realm_count;
} PWowAuthResult;

//logs into a vanilla (1.12.1, build 5875) realmd at host:port over SRP6, and
//asks it for its realm list. blocking: does not return until the exchange
//finishes or fails. out->error is set on failure and left empty on success
bool pe_wowauth_login(const char *host, int port, const char *account,
                      const char *password, PWowAuthResult *out);

#endif
