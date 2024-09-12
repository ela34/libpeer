#ifndef SDP_H_
#define SDP_H_

#include <string.h>
#include <stdint.h>
#include "config.h"

#define SDP_CONTENT_LENGTH 10240
#define SDP_ATTR_LENGTH 128

#ifndef ICE_LITE
#define ICE_LITE 0
#endif

typedef struct Sdp {
  char content[SDP_CONTENT_LENGTH];

} Sdp;

#define MAX_SDP_LINE_LENGTH  256
#define MAX_BUNDLE_IDS 4              // Maximum number of bundle IDs to store
#define MAX_BUNDLE_ID_LENGTH 12        // Maximum length of bundle ID
#define MAX_BUNDLE_ID_NAME_LENGTH 12  // Maximum number of bundle ID name

typedef struct SdpBundleId {

  uint32_t ssrc;
  char mid_name[MAX_BUNDLE_ID_LENGTH];
  char mid_type[MAX_BUNDLE_ID_NAME_LENGTH];

} SdpBundleId;

typedef struct SdpBundleInfo {

  int parsed;
  int count;
  SdpBundleId info[MAX_BUNDLE_IDS];

} SdpBundleInfo;

struct PeerConfiguration;
struct PeerConnection;
struct DtlsSrtp;
struct Agent;

void sdp_append_h264(Sdp* sdp, const char* mid, uint32_t ssrc);

void sdp_append_pcma(Sdp* sdp, const char* mid, uint32_t ssrc);

void sdp_append_pcmu(Sdp* sdp, const char* mid, uint32_t ssrc);

void sdp_append_opus(Sdp* sdp, const char* mid, uint32_t ssrc);

void sdp_append_datachannel(Sdp* sdp, const char* mid);

void sdp_create(Sdp *sdp, struct PeerConfiguration *config, SdpBundleInfo *bundle_info, struct DtlsSrtp *dtls_srtp, const char* role, char description[CONFIG_MTU]);

int sdp_append(Sdp* sdp, const char* format, ...);

void sdp_reset(Sdp* sdp);

void sdp_parse(struct Agent* agent, struct DtlsSrtp* dtls_srtp, SdpBundleInfo* bundle_info, int* role, const char* sdp_text);

#endif // SDP_H_
