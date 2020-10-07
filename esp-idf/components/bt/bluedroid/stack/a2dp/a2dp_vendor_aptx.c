/*
 * a2dp_vendor_aptx.c
 *
 *  Created on: 2018/12/25
 *      Author: nishi
 */
#include "common/bt_target.h"

#include <string.h>
#include "stack/a2d_api.h"
#include "a2d_int.h"
#include "stack/a2d_sbc.h"
#include "common/bt_defs.h"

#include "stack/a2dp_vendor_aptx.h"

// add by nishi
#include "common/bt_trace.h"


#if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE)

/* add by nishi start */
#define A2DP_BitsSet A2D_BitsSet
/* add by nishi end */


/* aptX Source codec capabilities */
static const tA2DP_APTX_CIE a2dp_aptx_source_caps = {
    A2DP_APTX_VENDOR_ID,                                       /* vendorId */
    A2DP_APTX_CODEC_ID_BLUETOOTH,                              /* codecId */
    (A2DP_APTX_SAMPLERATE_44100 | A2DP_APTX_SAMPLERATE_48000), /* sampleRate */
    A2DP_APTX_CHANNELS_STEREO,                                 /* channelMode */
    A2DP_APTX_FUTURE_1,                                        /* future1 */
    A2DP_APTX_FUTURE_2,                                        /* future2 */
    BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16 /* bits_per_sample */
};

/* Default aptX codec configuration */
static const tA2DP_APTX_CIE a2dp_aptx_default_config = {
    A2DP_APTX_VENDOR_ID,               /* vendorId */
    A2DP_APTX_CODEC_ID_BLUETOOTH,      /* codecId */
    A2DP_APTX_SAMPLERATE_48000,        /* sampleRate */
    A2DP_APTX_CHANNELS_STEREO,         /* channelMode */
    A2DP_APTX_FUTURE_1,                /* future1 */
    A2DP_APTX_FUTURE_2,                /* future2 */
    BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16 /* bits_per_sample */
};

/* encoder インターフェース なので無用? */
/* static const tA2DP_ENCODER_INTERFACE a2dp_encoder_interface_aptx = {
    a2dp_vendor_aptx_encoder_init,
    a2dp_vendor_aptx_encoder_cleanup,
    a2dp_vendor_aptx_feeding_reset,
    a2dp_vendor_aptx_feeding_flush,
    a2dp_vendor_aptx_get_encoder_interval_ms,
    a2dp_vendor_aptx_send_frames,
    nullptr  // set_transmit_queue_length
}; */


/*
 * Builds the aptX Media Codec Capabilities byte sequence beginning from the
 * LOSC octet. |media_type| is the media type |AVDT_MEDIA_TYPE_*|.
 * |p_ie| is a pointer to the aptX Codec Information Element information.
 * The result is stored in |p_result|. Returns A2DP_SUCCESS on success,
 * otherwise the corresponding A2DP error status code.
 */
tA2DP_STATUS A2DP_BuildInfoAptx(uint8_t media_type,const tA2DP_APTX_CIE* p_ie,uint8_t* p_result)
{
  if (p_ie == NULL || p_result == NULL) {
    return A2DP_INVALID_PARAMS;
  }

  *p_result++ = A2DP_APTX_CODEC_LEN;
  *p_result++ = (media_type << 4);
  *p_result++ = A2DP_MEDIA_CT_NON_A2DP;
  *p_result++ = (uint8_t)(p_ie->vendorId & 0x000000FF);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0x0000FF00) >> 8);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0x00FF0000) >> 16);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0xFF000000) >> 24);
  *p_result++ = (uint8_t)(p_ie->codecId & 0x00FF);
  *p_result++ = (uint8_t)((p_ie->codecId & 0xFF00) >> 8);
  *p_result++ = p_ie->sampleRate | p_ie->channelMode;

  return A2DP_SUCCESS;
}


// Parses the aptX Media Codec Capabilities byte sequence beginning from the
// LOSC octet. The result is stored in |p_ie|. The byte sequence to parse is
// |p_codec_info|. If |is_capability| is true, the byte sequence is
// codec capabilities, otherwise is codec configuration.
// Returns A2DP_SUCCESS on success, otherwise the corresponding A2DP error
// status code.
//tA2D_STATUS A2DP_ParseInfoAptx(tA2DP_APTX_CIE* p_ie,const uint8_t* p_codec_info,bool is_capability)
tA2D_STATUS A2DP_ParseInfoAptx(tA2DP_APTX_CIE* p_ie,const uint8_t* p_codec_info,bool is_capability,char *caller_s)
{
  uint8_t losc;
  uint8_t media_type;
  tA2DP_CODEC_TYPE codec_type;

  // add by nishi
  APPL_TRACE_WARNING("a2d_vender_aptx.c::A2DP_ParseInfoAptx() : #1 ,caller_s=%s",caller_s);


  if (p_ie == NULL || p_codec_info == NULL) return A2DP_INVALID_PARAMS;

  // Check the codec capability length
  losc = *p_codec_info++;
  if (losc != A2DP_APTX_CODEC_LEN) return A2DP_WRONG_CODEC;

  media_type = (*p_codec_info++) >> 4;
  codec_type = *p_codec_info++;
  /* Check the Media Type and Media Codec Type */
  //if (media_type != AVDT_MEDIA_TYPE_AUDIO ||
  if (media_type != AVDT_MEDIA_AUDIO ||
      codec_type != A2DP_MEDIA_CT_NON_A2DP) {
    return A2DP_WRONG_CODEC;
  }

  // Check the Vendor ID and Codec ID */
  p_ie->vendorId = (*p_codec_info & 0x000000FF) |
                   (*(p_codec_info + 1) << 8 & 0x0000FF00) |
                   (*(p_codec_info + 2) << 16 & 0x00FF0000) |
                   (*(p_codec_info + 3) << 24 & 0xFF000000);
  p_codec_info += 4;
  p_ie->codecId =
      (*p_codec_info & 0x00FF) | (*(p_codec_info + 1) << 8 & 0xFF00);
  p_codec_info += 2;
  if (p_ie->vendorId != A2DP_APTX_VENDOR_ID ||
      p_ie->codecId != A2DP_APTX_CODEC_ID_BLUETOOTH) {
    return A2DP_WRONG_CODEC;
  }

  p_ie->channelMode = *p_codec_info & 0x0F;
  p_ie->sampleRate = *p_codec_info & 0xF0;
  p_codec_info++;

  if (is_capability) {
    // NOTE: The checks here are very liberal. We should be using more
    // pedantic checks specific to the SRC or SNK as specified in the spec.
    if (A2DP_BitsSet(p_ie->sampleRate) == A2DP_SET_ZERO_BIT)
      return A2DP_BAD_SAMP_FREQ;
    if (A2DP_BitsSet(p_ie->channelMode) == A2DP_SET_ZERO_BIT)
      return A2DP_BAD_CH_MODE;

    return A2DP_SUCCESS;
  }

  if (A2DP_BitsSet(p_ie->sampleRate) != A2DP_SET_ONE_BIT)
    return A2DP_BAD_SAMP_FREQ;
  if (A2DP_BitsSet(p_ie->channelMode) != A2DP_SET_ONE_BIT)
    return A2DP_BAD_CH_MODE;

  return A2DP_SUCCESS;
}

/*******************************************************************************
**
** Function         bta_av_aptx_cfg_in_cap
**
** Description      This function checks whether an APTX codec configuration
**                  is allowable for the given codec capabilities.
**
** Returns          0 if ok, nonzero if error.
**
*******************************************************************************/
UINT8 bta_av_aptx_cfg_in_cap(UINT8 *p_cfg, tA2DP_APTX_CIE *p_cap)
{
    UINT8           status = 0;
    tA2DP_APTX_CIE    cfg_cie;

	// add by nishi
    APPL_TRACE_DEBUG("%s(): #1 called!!",__func__);
    /* parse configuration */
    if ((status = A2DP_ParseInfoAptx(&cfg_cie, p_cfg, FALSE,"bta_av_aptx_cfg_in_cap()")) != 0) {
        return status;
    }

    /* verify that each parameter is in range */

    /* sampling frequency */
    if ((cfg_cie.sampleRate & p_cap->sampleRate) == 0) {
        status = A2D_NS_SAMP_FREQ;
    }
    /* channel mode */
    else if ((cfg_cie.channelMode & p_cap->channelMode) == 0) {
        status = A2D_NS_CH_MODE;
    }
    /* block length */
    //else if ((cfg_cie.block_len & p_cap->block_len) == 0) {
    //    status = A2D_BAD_BLOCK_LEN;
    //}
    /* subbands */
    //else if ((cfg_cie.num_subbands & p_cap->num_subbands) == 0) {
    //    status = A2D_NS_SUBBANDS;
    //}
    /* allocation method */
    //else if ((cfg_cie.alloc_mthd & p_cap->alloc_mthd) == 0) {
    //    status = A2D_NS_ALLOC_MTHD;
    //}
    /* max bitpool */
    //else if (cfg_cie.max_bitpool > p_cap->max_bitpool) {
    //    status = A2D_NS_MAX_BITPOOL;
    //}
    /* min bitpool */
    //else if (cfg_cie.min_bitpool < p_cap->min_bitpool) {
    //    status = A2D_NS_MIN_BITPOOL;
    //}

    return status;
}


#endif /* #if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE) */
