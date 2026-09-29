#include "wowauth.h"
#include "wow_wire.h"

#include <engine/log.h>

#include <openssl/bn.h>
#include <openssl/rand.h>

#include <ctype.h>

//AUTH_LOGON_CHALLENGE/PROOF/REALM_LIST, the only opcodes a login needs
#define OP_LOGON_CHALLENGE 0x00
#define OP_LOGON_PROOF 0x01
#define OP_REALM_LIST 0x10

//---------------------------------------------------------------------------
//little-endian bignums
//
//every SRP value on the wire is least-significant byte first; OpenSSL's
//BIGNUM only reads/writes big-endian, so every value is byte-reversed on the
//way in and out. two sizes matter: a "natural" one (BN_num_bytes, no padding)
//used inside every hash the protocol takes, and a "fixed" one, zero-padded
//to an exact width, used only for what actually goes on the wire (A) and for
//S right before it is split into K. Getting these two mixed up is silent:
//the exchange still runs to completion, M1 just never matches the server's.
//---------------------------------------------------------------------------

static BIGNUM *bn_from_le(const u8 *bytes, int len) {
  u8 tmp[512];
  memcpy(tmp, bytes, len);
  ww_reverse_bytes(tmp, len);
  return BN_bin2bn(tmp, len, NULL);
}

//natural (minimal, unpadded) little-endian bytes of bn. returns the length
//written
static int bn_to_le_natural(const BIGNUM *bn, u8 *out) {
  int len = BN_num_bytes(bn);
  BN_bn2bin(bn, out);
  ww_reverse_bytes(out, len);
  return len;
}

//exactly `len` little-endian bytes, zero padded on the high (trailing) end
static void bn_to_le_fixed(const BIGNUM *bn, u8 *out, int len) {
  BN_bn2binpad(bn, out, len);
  ww_reverse_bytes(out, len);
}

//a 4-byte field the server reads as a c-string then reverses, so the client
//has to send it reversed and null-padded to begin with. "Win" -> 'n','i','W',0
static void wbuf_fourcc(WBuf *b, const char *str) {
  u8 buf[4] = {0, 0, 0, 0};
  int len = (int)strlen(str);
  if (len > 4)
    len = 4;
  for (int i = 0; i < len; i++)
    buf[i] = (u8)str[len - 1 - i];
  wbuf_bytes(b, buf, 4);
}

//---------------------------------------------------------------------------
//srp6, as WoW's auth protocol uses it: k=3 fixed (not H(N|g)), and every hash
//output is read back as a little-endian integer rather than the RFC's
//big-endian. N, g and the salt all come from the server's own challenge, so
//none of them are hardcoded here.
//---------------------------------------------------------------------------

typedef struct Srp {
  BIGNUM *N, *g, *k, *s, *a, *A, *B, *x, *u, *S;
  BN_CTX *ctx;
  u8 K[40]; //interleaved session key
  u8 M1[20];
  u8 M2[20]; //expected server proof
} Srp;

static void srp_free(Srp *srp) {
  BN_free(srp->N);
  BN_free(srp->g);
  BN_free(srp->k);
  BN_free(srp->s);
  BN_free(srp->a);
  BN_free(srp->A);
  BN_free(srp->B);
  BN_free(srp->x);
  BN_free(srp->u);
  BN_free(srp->S);
  BN_CTX_free(srp->ctx);
}

static void to_upper(char *s) {
  for (; *s; s++)
    *s = (char)toupper((unsigned char)*s);
}

//computes everything from the server's challenge through the two proofs.
//returns false only if the RNG could not produce a usable ephemeral, which
//in practice never happens
static void srp_compute(Srp *srp, const char *account, const char *password,
                        const u8 *B, int b_len, const u8 *g, int g_len,
                        const u8 *N, int n_len, const u8 *salt, int salt_len) {
  srp->ctx = BN_CTX_new();
  srp->N = bn_from_le(N, n_len);
  srp->g = bn_from_le(g, g_len);
  srp->k = BN_new();
  BN_set_word(srp->k, 3);
  srp->s = bn_from_le(salt, salt_len);
  srp->B = bn_from_le(B, b_len);
  srp->a = BN_new();
  srp->A = BN_new();
  srp->u = BN_new();
  srp->S = BN_new();

  char upper_account[64];
  snprintf(upper_account, sizeof(upper_account), "%s", account);
  to_upper(upper_account);
  char upper_password[128];
  snprintf(upper_password, sizeof(upper_password), "%s", password);
  to_upper(upper_password);

  //x = H(s | H(I:P)), both hash outputs read little-endian
  char combined[64 + 1 + 128];
  snprintf(combined, sizeof(combined), "%s:%s", upper_account, upper_password);
  u8 auth_hash[20];
  ww_sha1((const u8 *)combined, (int)strlen(combined), auth_hash);

  u8 x_input[32 + 20];
  memcpy(x_input, salt, salt_len);
  memcpy(x_input + salt_len, auth_hash, 20);
  u8 x_hash[20];
  ww_sha1(x_input, salt_len + 20, x_hash);
  srp->x = bn_from_le(x_hash, 20);

  //client ephemeral: a is 152 bits (19 bytes), matching the real client. A
  //must not be 0 mod N, though at these sizes that is a 2^-152 event
  for (int attempt = 0; attempt < 100; attempt++) {
    u8 rand_bytes[19];
    RAND_bytes(rand_bytes, sizeof(rand_bytes));
    BN_free(srp->a);
    srp->a = BN_bin2bn(rand_bytes, sizeof(rand_bytes), NULL);

    BN_free(srp->A);
    srp->A = BN_new();
    BN_mod_exp(srp->A, srp->g, srp->a, srp->N, srp->ctx);

    BIGNUM *a_mod_n = BN_new();
    BN_nnmod(a_mod_n, srp->A, srp->N, srp->ctx);
    bool nonzero = !BN_is_zero(a_mod_n);
    BN_free(a_mod_n);
    if (nonzero)
      break;
  }

  //u = H(A | B), natural (unpadded) sizes, little-endian
  u8 a_nat[512], b_nat[512];
  int a_nat_len = bn_to_le_natural(srp->A, a_nat);
  int b_nat_len = bn_to_le_natural(srp->B, b_nat);
  u8 ab[1024];
  memcpy(ab, a_nat, a_nat_len);
  memcpy(ab + a_nat_len, b_nat, b_nat_len);
  u8 u_hash[20];
  ww_sha1(ab, a_nat_len + b_nat_len, u_hash);
  BN_free(srp->u);
  srp->u = bn_from_le(u_hash, 20);

  //S = (B + kN - k*g^x) ^ (a + ux) mod N
  BIGNUM *gx = BN_new();
  BN_mod_exp(gx, srp->g, srp->x, srp->N, srp->ctx);
  BIGNUM *kgx = BN_new();
  BN_mul(kgx, srp->k, gx, srp->ctx);
  BIGNUM *kN = BN_new();
  BN_mul(kN, srp->k, srp->N, srp->ctx);
  BIGNUM *diff = BN_new();
  BN_add(diff, srp->B, kN);
  BN_sub(diff, diff, kgx);
  BIGNUM *ux = BN_new();
  BN_mul(ux, srp->u, srp->x, srp->ctx);
  BIGNUM *aux = BN_new();
  BN_add(aux, srp->a, ux);
  BN_mod_exp(srp->S, diff, aux, srp->N, srp->ctx);
  BN_free(gx);
  BN_free(kgx);
  BN_free(kN);
  BN_free(diff);
  BN_free(ux);
  BN_free(aux);

  //K: split S's 32 fixed little-endian bytes into even/odd halves, hash each,
  //interleave the two 20-byte digests back together
  u8 s_fixed[32];
  bn_to_le_fixed(srp->S, s_fixed, 32);
  u8 s1[16], s2[16];
  for (int i = 0; i < 16; i++) {
    s1[i] = s_fixed[i * 2];
    s2[i] = s_fixed[i * 2 + 1];
  }
  u8 s1_hash[20], s2_hash[20];
  ww_sha1(s1, 16, s1_hash);
  ww_sha1(s2, 16, s2_hash);
  for (int i = 0; i < 20; i++) {
    srp->K[i * 2] = s1_hash[i];
    srp->K[i * 2 + 1] = s2_hash[i];
  }

  //M1 = H( H(N)^H(g) | H(I) | s | A | B | K ), all natural sizes
  int n_nat_len = BN_num_bytes(srp->N);
  int g_nat_len = BN_num_bytes(srp->g);
  u8 n_nat[512], g_nat[512];
  bn_to_le_natural(srp->N, n_nat);
  bn_to_le_natural(srp->g, g_nat);
  u8 n_hash[20], g_hash[20];
  ww_sha1(n_nat, n_nat_len, n_hash);
  ww_sha1(g_nat, g_nat_len, g_hash);
  u8 ng_xor[20];
  for (int i = 0; i < 20; i++)
    ng_xor[i] = n_hash[i] ^ g_hash[i];

  u8 user_hash[20];
  ww_sha1((const u8 *)upper_account, (int)strlen(upper_account), user_hash);

  u8 s_nat[512];
  int s_nat_len = bn_to_le_natural(srp->s, s_nat);
  //A and B were already computed above as a_nat/b_nat

  u8 m1_input[20 + 20 + 32 + 512 + 512 + 40];
  int p = 0;
  memcpy(m1_input + p, ng_xor, 20);
  p += 20;
  memcpy(m1_input + p, user_hash, 20);
  p += 20;
  memcpy(m1_input + p, s_nat, s_nat_len);
  p += s_nat_len;
  memcpy(m1_input + p, a_nat, a_nat_len);
  p += a_nat_len;
  memcpy(m1_input + p, b_nat, b_nat_len);
  p += b_nat_len;
  memcpy(m1_input + p, srp->K, 40);
  p += 40;
  ww_sha1(m1_input, p, srp->M1);

  //M2 = H( A | M1 | K )
  u8 m2_input[512 + 20 + 40];
  p = 0;
  memcpy(m2_input + p, a_nat, a_nat_len);
  p += a_nat_len;
  memcpy(m2_input + p, srp->M1, 20);
  p += 20;
  memcpy(m2_input + p, srp->K, 40);
  p += 40;
  ww_sha1(m2_input, p, srp->M2);
}

//---------------------------------------------------------------------------
//the login exchange
//---------------------------------------------------------------------------

static void fail(PWowAuthResult *out, const char *msg) {
  out->success = false;
  snprintf(out->error, PE_WOWAUTH_ERROR_MAX, "%s", msg);
}

static const char *auth_result_string(u8 code) {
  switch (code) {
  case 0x03:
    return "account banned";
  case 0x04:
    return "unknown account";
  case 0x05:
    return "incorrect password";
  case 0x06:
    return "account already online";
  case 0x0C:
    return "account suspended";
  default:
    return "auth server rejected login";
  }
}

bool pe_wowauth_login(const char *host, int port, const char *account,
                      const char *password, PWowAuthResult *out) {
  memset(out, 0, sizeof(*out));

  int fd = tcp_connect(host, port);
  if (fd < 0) {
    fail(out, "could not connect to realmd");
    return false;
  }

  //LOGON_CHALLENGE: protocol version 3 (legacy vanilla, not the v8 PIN/token
  //extensions), build 5875 (1.12.1), account name uppercased
  char upper_account[64];
  snprintf(upper_account, sizeof(upper_account), "%s", account);
  to_upper(upper_account);
  int account_len = (int)strlen(upper_account);

  WBuf out_buf;
  out_buf.len = 0;
  wbuf_u8(&out_buf, OP_LOGON_CHALLENGE);
  wbuf_u8(&out_buf, 3); //protocol version
  wbuf_u16(&out_buf, (u16)(30 + account_len));
  wbuf_fourcc(&out_buf, "WoW");
  wbuf_u8(&out_buf, 1);  //major
  wbuf_u8(&out_buf, 12); //minor
  wbuf_u8(&out_buf, 1);  //patch
  wbuf_u16(&out_buf, 5875);
  wbuf_fourcc(&out_buf, "x86");
  wbuf_fourcc(&out_buf, "Win");
  wbuf_fourcc(&out_buf, "enUS");
  wbuf_u32(&out_buf, 0); //timezone
  wbuf_u32(&out_buf, 0); //client ip, unchecked by the server
  wbuf_u8(&out_buf, (u8)account_len);
  wbuf_bytes(&out_buf, (const u8 *)upper_account, account_len);

  if (!send_all(fd, out_buf.data, out_buf.len)) {
    fail(out, "failed sending LOGON_CHALLENGE");
    close(fd);
    return false;
  }

  //response: opcode, protocol echo, status, then only present on success
  u8 hdr[3];
  if (!read_exact(fd, hdr, 3) || hdr[0] != OP_LOGON_CHALLENGE) {
    fail(out, "malformed LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  if (hdr[2] != 0) {
    fail(out, auth_result_string(hdr[2]));
    close(fd);
    return false;
  }

  u8 B[32];
  if (!read_exact(fd, B, 32)) {
    fail(out, "truncated LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  u8 g_len;
  if (!read_exact(fd, &g_len, 1)) {
    fail(out, "truncated LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  u8 g[32];
  if (!read_exact(fd, g, g_len)) {
    fail(out, "truncated LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  u8 n_len;
  if (!read_exact(fd, &n_len, 1)) {
    fail(out, "truncated LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  u8 N[256];
  if (!read_exact(fd, N, n_len)) {
    fail(out, "truncated LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  u8 salt[32];
  if (!read_exact(fd, salt, 32)) {
    fail(out, "truncated LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  u8 checksum_salt[16];
  if (!read_exact(fd, checksum_salt, 16)) {
    fail(out, "truncated LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  u8 security_flags;
  if (!read_exact(fd, &security_flags, 1)) {
    fail(out, "truncated LOGON_CHALLENGE response");
    close(fd);
    return false;
  }
  //PIN/matrix/authenticator extensions: skip their bytes to stay framed, we
  //don't support any of them
  if (security_flags & 0x01) {
    u8 skip[20];
    read_exact(fd, skip, 20);
  }
  if (security_flags & 0x02) {
    u8 skip[12];
    read_exact(fd, skip, 12);
  }
  if (security_flags & 0x04) {
    u8 skip[1];
    read_exact(fd, skip, 1);
  }

  Srp srp;
  memset(&srp, 0, sizeof(srp));
  srp_compute(&srp, upper_account, password, B, 32, g, g_len, N, n_len, salt,
             32);

  //LOGON_PROOF: A(32, fixed-width wire form) + M1(20) + crc(20, zero - we
  //have no real client binary to hash) + numkeys(1) + security flags(1).
  //realmd reads this as one fixed struct that ends in the flags byte even
  //for a legacy (non-PIN) proof, so it can't be left off
  u8 A_fixed[32];
  bn_to_le_fixed(srp.A, A_fixed, 32);

  WBuf proof;
  proof.len = 0;
  wbuf_u8(&proof, OP_LOGON_PROOF);
  wbuf_bytes(&proof, A_fixed, 32);
  wbuf_bytes(&proof, srp.M1, 20);
  u8 zero20[20];
  memset(zero20, 0, 20);
  wbuf_bytes(&proof, zero20, 20); //crc hash
  wbuf_u8(&proof, 0);             //number of keys
  wbuf_u8(&proof, 0);             //security flags

  if (!send_all(fd, proof.data, proof.len)) {
    fail(out, "failed sending LOGON_PROOF");
    srp_free(&srp);
    close(fd);
    return false;
  }

  u8 proof_hdr[2];
  if (!read_exact(fd, proof_hdr, 2) || proof_hdr[0] != OP_LOGON_PROOF) {
    fail(out, "malformed LOGON_PROOF response");
    srp_free(&srp);
    close(fd);
    return false;
  }
  if (proof_hdr[1] != 0) {
    fail(out, auth_result_string(proof_hdr[1]));
    srp_free(&srp);
    close(fd);
    return false;
  }

  //build < 6299 (ours is 5875): M2(20) + surveyId(4), nothing more
  u8 m2[20];
  u8 survey_id[4];
  if (!read_exact(fd, m2, 20) || !read_exact(fd, survey_id, 4)) {
    fail(out, "truncated LOGON_PROOF response");
    srp_free(&srp);
    close(fd);
    return false;
  }
  if (memcmp(m2, srp.M2, 20) != 0) {
    fail(out, "server proof did not match (wrong password, or a protocol "
              "mismatch)");
    srp_free(&srp);
    close(fd);
    return false;
  }

  memcpy(out->session_key, srp.K, 40);
  srp_free(&srp);

  //REALM_LIST: opcode + an unused u32
  WBuf realm_req;
  realm_req.len = 0;
  wbuf_u8(&realm_req, OP_REALM_LIST);
  wbuf_u32(&realm_req, 0);
  if (!send_all(fd, realm_req.data, realm_req.len)) {
    fail(out, "failed requesting realm list");
    close(fd);
    return false;
  }

  u8 realm_hdr[3];
  if (!read_exact(fd, realm_hdr, 3) || realm_hdr[0] != OP_REALM_LIST) {
    fail(out, "malformed REALM_LIST response");
    close(fd);
    return false;
  }
  u16 payload_size = (u16)(realm_hdr[1] | (realm_hdr[2] << 8));

  WBuf realm_buf;
  realm_buf.len = payload_size;
  realm_buf.pos = 0;
  if (!read_exact(fd, realm_buf.data, payload_size)) {
    fail(out, "truncated REALM_LIST response");
    close(fd);
    return false;
  }

  wbuf_read_u32(&realm_buf); //unused
  u8 realm_count = wbuf_read_u8(&realm_buf); //vanilla: u8, not u16

  out->realm_count = 0;
  for (int i = 0; i < realm_count && i < PE_WOWAUTH_REALMS_MAX; i++) {
    PWowRealm *realm = &out->realms[out->realm_count];

    //vanilla layout: icon is a u32, and there is no lock byte
    realm->icon = (u8)wbuf_read_u32(&realm_buf);
    realm->flags = wbuf_read_u8(&realm_buf);
    wbuf_read_string(&realm_buf, realm->name, sizeof(realm->name));
    wbuf_read_string(&realm_buf, realm->address, sizeof(realm->address));

    u32 population_bits = wbuf_read_u32(&realm_buf);
    memcpy(&realm->population, &population_bits, sizeof(float));

    wbuf_read_u8(&realm_buf); //characters on this realm for this account
    wbuf_read_u8(&realm_buf); //timezone
    realm->id = wbuf_read_u8(&realm_buf);

    if (realm->flags & 0x04) { //version info, unused here
      wbuf_read_u8(&realm_buf);
      wbuf_read_u8(&realm_buf);
      wbuf_read_u8(&realm_buf);
      wbuf_read_u16(&realm_buf);
    }

    out->realm_count++;
  }

  close(fd);
  out->success = true;
  return true;
}
