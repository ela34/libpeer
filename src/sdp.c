#include <stdio.h>
#include <stdarg.h>

#include "sdp.h"
#include "rtp.h"
#include "dtls_srtp.h"
#include "peer_connection.h"
#include "utils.h"

void sdp_parse_bundle_line(const char* line, SdpBundleInfo *bundle_info) {
  const char* token;
  char line_copy[MAX_SDP_LINE_LENGTH];  // Buffer to make a copy of the input line

  bundle_info->count = 0;  // Initialize the count of bundle IDs

  // Make a copy of the input line to tokenize
  strncpy(line_copy, line, sizeof(line_copy) - 1);
  line_copy[sizeof(line_copy) - 1] = '\0';  // Ensure null-terminated

  // Skip the prefix
  token = strtok(line_copy, " ");

  // Parse each token (media ID) and store it as an integer
  while ((token != NULL) && (bundle_info->count < MAX_BUNDLE_IDS) && (strlen(token) < MAX_BUNDLE_ID_LENGTH)) {
    strcpy(bundle_info->info[bundle_info->count].mid_name, token);
    bundle_info->info[bundle_info->count].mid_type[0] = 0;
    bundle_info->info[bundle_info->count].ssrc = 0;
    bundle_info->count++;
    token = strtok(NULL, " ");
  }
}

void sdp_set_bundle_info(SdpBundleInfo *bundle_info, char mid_name[MAX_BUNDLE_ID_LENGTH], char mid_type[MAX_BUNDLE_ID_NAME_LENGTH], uint32_t ssrc_id) {
  LOGD("Adding %s,%s,%lu\n", mid_name, mid_type, ssrc_id);
  for (int i = 0 ; i < bundle_info->count ; i++) {
    if (strcmp(bundle_info->info[i].mid_name, mid_name) == 0) {
      strcpy(bundle_info->info[i].mid_type, mid_type);
      bundle_info->info[i].ssrc = ssrc_id;
      LOGD("Added[%d] %s,%s,%lu\n", i, mid_name, mid_type, ssrc_id);
      break;
    }
  }
}

void sdp_parse(SdpBundleInfo *bundle_info, const char *sdp_text) {
  char *start = (char*)sdp_text;
  char *line = NULL;
  char buf[MAX_SDP_LINE_LENGTH], mid_type[MAX_BUNDLE_ID_NAME_LENGTH], mid_name[MAX_BUNDLE_ID_LENGTH];
  size_t len;
  char *end;
  int has_info = 0;
  uint32_t ssrc_id = 0;

  mid_type[0] = mid_name[0] = 0;
  bundle_info->count = 0;
  while ((line = strstr(start, "\r\n")) != NULL) {
    len = line - start;
    if (len < MAX_SDP_LINE_LENGTH) {
      strncpy(buf, start, len);
      buf[len] = '\0';

      if (strstr(buf, "a=group:BUNDLE") != NULL)
        sdp_parse_bundle_line(&buf[14], bundle_info);

      if (strstr(buf, "m=")) {
        if (has_info)
          sdp_set_bundle_info(bundle_info, mid_name, mid_type, ssrc_id);

        has_info = 0;
        if ((end = strstr(&buf[2], " ")) != NULL) {
          len = end - &buf[2];
          if (len < MAX_BUNDLE_ID_NAME_LENGTH) {
            strncpy(mid_type, &buf[2], len);
            mid_type[len] = '\0';
            LOGD("m= '%s'\n", mid_type);
            mid_name[0] = 0;
            ssrc_id = 0;
            has_info = 1;
          }
        }
      }

      if ((strstr(buf, "a=mid:") != NULL) && (mid_type[0] != 0)) {
        if (strlen(&buf[6]) < MAX_BUNDLE_ID_LENGTH)
          strcpy(mid_name, &buf[6]);
        LOGD("a=mid: '%s'\n", mid_name);
      }

      if (strstr(buf, "a=ssrc:") != NULL) {
        ssrc_id = strtoul(&buf[7], NULL, 10);
        LOGD("a=ssrc: '%s' = %lu\n", &buf[7], ssrc_id);
      }
    }
    start = line + 2;
  }

  if (has_info)
    sdp_set_bundle_info(bundle_info, mid_name, mid_type, ssrc_id);

  LOGI("Detected %d bundle entries", bundle_info->count);
  for (int i = 0 ; i < bundle_info->count ; i++)
    LOGI("%d] type='%s' name='%s' ssrc=%lu", i, bundle_info->info[i].mid_type, bundle_info->info[i].mid_name, (unsigned long) bundle_info->info[i].ssrc);
}

void sdp_append_bundle(Sdp *sdp, struct PeerConfiguration *config, SdpBundleInfo *bundle_info) {

  char bundle[16 + ((MAX_BUNDLE_ID_LENGTH + 1) * MAX_BUNDLE_IDS)];
  char *name = NULL;

  sdp_append(sdp, "v=0");
  sdp_append(sdp, "o=- 1495799811084970 1495799811084970 IN IP4 0.0.0.0");
  sdp_append(sdp, "s=-");
  sdp_append(sdp, "t=0 0");

  memset(bundle, 0, sizeof(bundle));

  strcat(bundle, "a=group:BUNDLE");
  for (int i = 0; i < bundle_info->count ; i++) {
    if ((strcmp(bundle_info->info[i].mid_type, "audio") == 0) && (config->audio_codec != CODEC_NONE))
      name = bundle_info->info[i].mid_name;
    else if ((strcmp(bundle_info->info[i].mid_type, "video") == 0) && (config->video_codec != CODEC_NONE))
      name = bundle_info->info[i].mid_name;
    else if ((strcmp(bundle_info->info[i].mid_type, "application") == 0) && config->datachannel)
      name = bundle_info->info[i].mid_name;

    if (name != NULL) {
      strcat(bundle, " ");
      strcat(bundle, name);
      name = NULL;
    }
  }

  sdp_append(sdp, bundle);
  sdp_append(sdp, "a=msid-semantic: WMS myStream");
}

int sdp_append(Sdp *sdp, const char *format, ...) {

  va_list argptr;

  char attr[SDP_ATTR_LENGTH];

  memset(attr, 0, sizeof(attr));

  va_start(argptr, format);

  vsnprintf(attr, sizeof(attr), format, argptr);

  va_end(argptr);

  strcat(sdp->content, attr);
  strcat(sdp->content, "\r\n");
  return 0;
}

void sdp_reset(Sdp *sdp) {

  memset(sdp->content, 0, sizeof(sdp->content));
}

void sdp_append_h264(Sdp *sdp, const char *mid, uint32_t ssrc) {

  sdp_append(sdp, "m=video 9 UDP/TLS/RTP/SAVPF 96 102");
  sdp_append(sdp, "a=rtcp-fb:102 nack");
  sdp_append(sdp, "a=rtcp-fb:102 nack pli");
  sdp_append(sdp, "a=fmtp:96 profile-level-id=42e01f;level-asymmetry-allowed=1");
  sdp_append(sdp, "a=fmtp:102 profile-level-id=42e01f;packetization-mode=1;level-asymmetry-allowed=1");
  sdp_append(sdp, "a=rtpmap:96 H264/90000");
  sdp_append(sdp, "a=rtpmap:102 H264/90000");
  sdp_append(sdp, "a=ssrc:%lu cname:webrtc-h264", ssrc);
  sdp_append(sdp, "a=sendrecv");
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_pcma(Sdp *sdp, const char *mid, uint32_t ssrc) {

  sdp_append(sdp, "m=audio 9 UDP/TLS/RTP/SAVP 8");
  sdp_append(sdp, "a=rtpmap:8 PCMA/8000");
  sdp_append(sdp, "a=ssrc:%lu cname:webrtc-pcma", ssrc);
  sdp_append(sdp, "a=sendrecv");
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_pcmu(Sdp *sdp, const char *mid, uint32_t ssrc) {

  sdp_append(sdp, "m=audio 9 UDP/TLS/RTP/SAVP 0");
  sdp_append(sdp, "a=rtpmap:0 PCMU/8000");
  sdp_append(sdp, "a=ssrc:%lu cname:webrtc-pcmu", ssrc);
  sdp_append(sdp, "a=sendrecv");
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_opus(Sdp *sdp, const char *mid, uint32_t ssrc) {

  sdp_append(sdp, "m=audio 9 UDP/TLS/RTP/SAVP 111");
  sdp_append(sdp, "a=rtpmap:111 opus/48000/2");
  sdp_append(sdp, "a=ssrc:%lu cname:webrtc-opus", ssrc);
  sdp_append(sdp, "a=sendrecv");
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_datachannel(Sdp *sdp, const char *mid) {

  sdp_append(sdp, "m=application 50712 UDP/DTLS/SCTP webrtc-datachannel");
  sdp_append(sdp, "a=mid:%s", mid);
  sdp_append(sdp, "a=sctp-port:5000");
  sdp_append(sdp, "c=IN IP4 0.0.0.0");
  sdp_append(sdp, "a=max-message-size:262144");
}


void sdp_create(Sdp *local_sdp, struct PeerConfiguration *config, SdpBundleInfo *bundle_info, DtlsSrtp *dtls_srtp, char description[CONFIG_MTU]) {
  memset(local_sdp, 0, sizeof(*local_sdp));
  // TODO: check if we have video or audio codecs
  sdp_append_bundle(local_sdp, config, bundle_info);

  for (int i = 0; i < bundle_info->count ; i++) {
    if ((strcmp(bundle_info->info[i].mid_type, "audio") == 0) && (config->audio_codec != CODEC_NONE)) {
      switch (config->audio_codec) {
        case CODEC_PCMA:
          sdp_append_pcma(local_sdp, bundle_info->info[i].mid_name, config->audio_ssrc);
          sdp_append(local_sdp, "a=fingerprint:sha-256 %s", dtls_srtp->local_fingerprint);
          sdp_append(local_sdp, "a=setup:passive");
          strcat(local_sdp->content, description);
          break;

        case CODEC_PCMU:
          sdp_append_pcmu(local_sdp, bundle_info->info[i].mid_name, config->audio_ssrc);
          sdp_append(local_sdp, "a=fingerprint:sha-256 %s", dtls_srtp->local_fingerprint);
          sdp_append(local_sdp, "a=setup:passive");
          strcat(local_sdp->content, description);
          break;

        case CODEC_OPUS:
          sdp_append_opus(local_sdp, bundle_info->info[i].mid_name, config->audio_ssrc);
          sdp_append(local_sdp, "a=fingerprint:sha-256 %s", dtls_srtp->local_fingerprint);
          sdp_append(local_sdp, "a=setup:passive");
          strcat(local_sdp->content, description);
          break;

        default:
          break;
      }
    } else if ((strcmp(bundle_info->info[i].mid_type, "video") == 0) && (config->video_codec == CODEC_H264)) {
      sdp_append_h264(local_sdp, bundle_info->info[i].mid_name, config->video_ssrc);
      sdp_append(local_sdp, "a=fingerprint:sha-256 %s", dtls_srtp->local_fingerprint);
      sdp_append(local_sdp, "a=setup:passive");
      strcat(local_sdp->content, description);
    }  else if ((strcmp(bundle_info->info[i].mid_type, "application") == 0) && config->datachannel) {
      sdp_append_datachannel(local_sdp, bundle_info->info[i].mid_name);
      sdp_append(local_sdp, "a=fingerprint:sha-256 %s", dtls_srtp->local_fingerprint);
      sdp_append(local_sdp, "a=setup:passive");
      strcat(local_sdp->content, description);
    }
  }
}

