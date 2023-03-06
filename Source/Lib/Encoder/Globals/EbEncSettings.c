/*
* Copyright(c) 2022 Intel Corporation
*
* This source code is subject to the terms of the BSD 3-Clause Clear License and
* the Alliance for Open Media Patent License 1.0. If the BSD 3-Clause Clear License
* was not distributed with this source code in the LICENSE file, you can
* obtain it at https://www.aomedia.org/license. If the Alliance for Open
* Media Patent License 1.0 was not distributed with this source code in the
* PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
*/
// SUMMARY
//   Contains the encoder settings API functions

/**************************************
 * Includes
 **************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "EbVersion.h"
#include "EbDefinitions.h"
#include "EbSvtAv1Enc.h"
#include "EbSvtAv1Metadata.h"
#include "EbEncSettings.h"

#include "EbLog.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#endif



/**********************************
Set Default Library Params
**********************************/
EbErrorType svt_av1_set_default_params(EbSvtAv1EncConfiguration *config_ptr) {
    EbErrorType return_error = EB_ErrorNone;

    if (!config_ptr) {
        SVT_ERROR("The EbSvtAv1EncConfiguration structure is empty!\n");
        return EB_ErrorBadParameter;
    }
    config_ptr->frame_rate_numerator      = 60000;
    config_ptr->frame_rate_denominator    = 1000;
    config_ptr->encoder_bit_depth         = 8;
    config_ptr->compressed_ten_bit_format = 0;
    config_ptr->source_width              = 0;
    config_ptr->source_height             = 0;
    config_ptr->forced_max_frame_width    = 0;
    config_ptr->forced_max_frame_height   = 0;
    config_ptr->stat_report               = 0;
    config_ptr->tile_rows                 = DEFAULT;
    config_ptr->tile_columns              = DEFAULT;
    config_ptr->qp                        = DEFAULT_QP;
    config_ptr->use_qp_file               = FALSE;
    config_ptr->use_fixed_qindex_offsets  = 0;
    memset(config_ptr->qindex_offsets, 0, sizeof(config_ptr->qindex_offsets));
    config_ptr->key_frame_chroma_qindex_offset = 0;
    config_ptr->key_frame_qindex_offset        = 0;
    memset(config_ptr->chroma_qindex_offsets, 0, sizeof(config_ptr->chroma_qindex_offsets));
    config_ptr->luma_y_dc_qindex_offset   = 0;
    config_ptr->chroma_u_dc_qindex_offset = 0;
    config_ptr->chroma_u_ac_qindex_offset = 0;
    config_ptr->chroma_v_dc_qindex_offset = 0;
    config_ptr->chroma_v_ac_qindex_offset = 0;

    for (int i = 0; i < SVT_AV1_FRAME_UPDATE_TYPES; i++) config_ptr->lambda_scale_factors[i] = 128;

    config_ptr->scene_change_detection = 0;
    config_ptr->rate_control_mode      = SVT_AV1_RC_MODE_CQP_OR_CRF;
    config_ptr->look_ahead_distance    = (uint32_t)~0;
    config_ptr->enable_tpl_la          = 1;
    config_ptr->target_bit_rate        = 2000000;
    config_ptr->max_bit_rate           = 0;
    config_ptr->max_qp_allowed         = 63;
    config_ptr->min_qp_allowed         = 1;

    config_ptr->enable_adaptive_quantization = 2;
    config_ptr->enc_mode                     = 10;
    config_ptr->intra_period_length          = -2;
    config_ptr->multiply_keyint              = FALSE;
    config_ptr->intra_refresh_type           = 2;
    config_ptr->hierarchical_levels          = 0;
    config_ptr->pred_structure               = SVT_AV1_PRED_RANDOM_ACCESS;
    config_ptr->enable_dlf_flag              = TRUE;
    config_ptr->cdef_level                   = DEFAULT;
    config_ptr->enable_restoration_filtering = DEFAULT;
    config_ptr->enable_mfmv                  = DEFAULT;
    config_ptr->enable_dg                    = 1;
    config_ptr->fast_decode                  = 0;
    config_ptr->encoder_color_format         = EB_YUV420;
    // Rate control options
    // Set the default value toward more flexible rate allocation
    config_ptr->vbr_bias_pct             = 100;
    config_ptr->vbr_min_section_pct      = 0;
    config_ptr->vbr_max_section_pct      = 2000;
    config_ptr->under_shoot_pct          = 25;
    config_ptr->over_shoot_pct           = 25;
    config_ptr->mbr_over_shoot_pct       = 50;
    config_ptr->gop_constraint_rc        = 0;
    config_ptr->maximum_buffer_size_ms   = 1000; // default settings for CBR
    config_ptr->starting_buffer_level_ms = 600; // default settings for CBR
    config_ptr->optimal_buffer_level_ms  = 600; // default settings for CBR
    config_ptr->recode_loop              = ALLOW_RECODE_DEFAULT;
    config_ptr->restricted_motion_vector = FALSE;

    config_ptr->high_dynamic_range_input = 0;
    config_ptr->screen_content_mode      = 2;

    // Annex A parameters
    config_ptr->profile = 0;
    config_ptr->tier    = 0;
    config_ptr->level   = 0;

    // Latency
    config_ptr->film_grain_denoise_strength = 0;
    config_ptr->film_grain_denoise_apply    = 1;

    // CPU Flags
    config_ptr->use_cpu_flags = EB_CPU_FLAGS_ALL;

    // Channel info
    config_ptr->logical_processors   = 0;
    config_ptr->pin_threads          = 0;
    config_ptr->target_socket        = -1;
    config_ptr->channel_id           = 0;
    config_ptr->active_channel_count = 1;

    // Debug info
    config_ptr->recon_enabled = 0;

    // Alt-Ref default values
    config_ptr->enable_tf       = TRUE;
    config_ptr->enable_overlays = FALSE;
    config_ptr->tune            = 1;
    // Super-resolution default values
    config_ptr->superres_mode      = SUPERRES_NONE;
    config_ptr->superres_denom     = SCALE_NUMERATOR;
    config_ptr->superres_kf_denom  = SCALE_NUMERATOR;
    config_ptr->superres_qthres    = 43; // random threshold, change
    config_ptr->superres_kf_qthres = 43; // random threshold, change

    // Reference Scaling default values
    config_ptr->resize_mode     = RESIZE_NONE;
    config_ptr->resize_denom    = SCALE_NUMERATOR;
    config_ptr->resize_kf_denom = SCALE_NUMERATOR;

    // Color description default values
    config_ptr->color_description_present_flag = FALSE;
    config_ptr->color_primaries                = 2;
    config_ptr->transfer_characteristics       = 2;
    config_ptr->matrix_coefficients            = 2;
    config_ptr->color_range                    = EB_CR_STUDIO_RANGE;
    config_ptr->chroma_sample_position         = EB_CSP_UNKNOWN;
    config_ptr->pass                           = 0;
    memset(&config_ptr->mastering_display, 0, sizeof(config_ptr->mastering_display));
    memset(&config_ptr->content_light_level, 0, sizeof(config_ptr->content_light_level));

    // Switch frame default values
    config_ptr->sframe_dist      = 0;
    config_ptr->sframe_mode      = SFRAME_NEAREST_BASE;
    config_ptr->force_key_frames = 0;

    // Quant Matrices (QM)
    config_ptr->enable_qm    = 0;
    config_ptr->min_qm_level = 8;
    config_ptr->max_qm_level = 15;

#if FTR_STARTUP_MG_SIZE
    config_ptr->startup_mg_size = 0;
#endif
    return return_error;
}

static const char *tier_to_str(unsigned in) {
    if (!in)
        return "(auto)";
    static char ret[11];
    snprintf(ret, 11, "%u", in);
    return ret;
}
static const char *level_to_str(unsigned in) {
    if (!in)
        return "(auto)";
    static char ret[313];
    snprintf(ret, 313, "%.1f", in / 10.0);
    return ret;
}

//#define DEBUG_BUFFERS
void svt_av1_print_lib_params(SequenceControlSet *scs) {
    EbSvtAv1EncConfiguration *config = &scs->static_config;

    SVT_INFO("-------------------------------------------\n");

    if (config->pass == ENC_FIRST_PASS)
        SVT_INFO("SVT [config]: preset \t\t\t\t\t\t\t: Pass 1\n");
    else if (config->pass == ENC_MIDDLE_PASS)
        SVT_INFO("SVT [config]: preset \t\t\t\t\t\t\t: Pass 2\n");
    else {
        SVT_INFO("SVT [config]: %s\ttier %s\tlevel %s\n",
                 config->profile == MAIN_PROFILE               ? "main profile"
                     : config->profile == HIGH_PROFILE         ? "high profile"
                     : config->profile == PROFESSIONAL_PROFILE ? "professional profile"
                                                               : "Unknown profile",
                 tier_to_str(config->tier),
                 level_to_str(config->level));
        SVT_INFO(
            "SVT [config]: width / height / fps numerator / fps denominator \t\t: %d / %d / %d / "
            "%d\n",
            config->source_width,
            config->source_height,
            config->frame_rate_numerator,
            config->frame_rate_denominator);
        SVT_INFO(
            "SVT [config]: bit-depth / color format \t\t\t\t\t: %d / "
            "%s\n",
            config->encoder_bit_depth,
            config->encoder_color_format == EB_YUV400       ? "YUV400"
                : config->encoder_color_format == EB_YUV420 ? "YUV420"
                : config->encoder_color_format == EB_YUV422 ? "YUV422"
                : config->encoder_color_format == EB_YUV444 ? "YUV444"
                                                            : "Unknown color format");

        SVT_INFO("SVT [config]: preset / tune / pred struct \t\t\t\t\t: %d / %s / %s\n",
                 config->enc_mode,
                 config->tune == 0 ? "VQ" : "PSNR",
                 config->pred_structure == 1       ? "low delay"
                     : config->pred_structure == 2 ? "random access"
                                                   : "Unknown pred structure");
        SVT_INFO(
            "SVT [config]: gop size / mini-gop size / key-frame type \t\t\t: "
            "%d / %d / %s\n",
            config->intra_period_length + 1,
            (1 << config->hierarchical_levels),
            config->intra_refresh_type == SVT_AV1_FWDKF_REFRESH    ? "FWD key frame"
                : config->intra_refresh_type == SVT_AV1_KF_REFRESH ? "key frame"
                                                                   : "Unknown key frame type");

        switch (config->rate_control_mode) {
        case SVT_AV1_RC_MODE_CQP_OR_CRF:
            if (config->max_bit_rate)
                SVT_INFO(
                    "SVT [config]: BRC mode / %s / max bitrate (kbps)\t\t\t: %s / %d / "
                    "%d\n",
                    scs->tpl_level ? "rate factor" : "CQP Assignment",
                    scs->tpl_level ? "capped CRF" : "CQP",
                    scs->static_config.qp,
                    (int)config->max_bit_rate / 1000);
            else
                SVT_INFO("SVT [config]: BRC mode / %s \t\t\t\t\t: %s / %d \n",
                         scs->tpl_level ? "rate factor" : "CQP Assignment",
                         scs->tpl_level ? "CRF" : "CQP",
                         scs->static_config.qp);
            break;
        case SVT_AV1_RC_MODE_VBR:
            SVT_INFO("SVT [config]: BRC mode / target bitrate (kbps)\t\t\t\t: VBR / %d \n",
                     (int)config->target_bit_rate / 1000);
            break;
        case SVT_AV1_RC_MODE_CBR:
            SVT_INFO(
                "SVT [config]: BRC mode / target bitrate (kbps)\t\t\t\t: CBR "
                "/ %d\n",
                (int)config->target_bit_rate / 1000);
            break;
        }

        if (config->film_grain_denoise_strength != 0) {
            SVT_INFO("SVT [config]: film grain synth / denoising / level \t\t\t\t: %d / %d / %d\n",
                     1,
                     config->film_grain_denoise_apply,
                     config->film_grain_denoise_strength);
        }
    }
#ifdef DEBUG_BUFFERS
    SVT_INFO("SVT [config]: INPUT / OUTPUT \t\t\t\t\t\t\t: %d / %d\n",
             scs->input_buffer_fifo_init_count,
             scs->output_stream_buffer_fifo_init_count);
    SVT_INFO("SVT [config]: CPCS / PAREF / REF / ME\t\t\t\t\t\t: %d / %d / %d / %d\n",
             scs->picture_control_set_pool_init_count_child,
             scs->pa_reference_picture_buffer_init_count,
             scs->reference_picture_buffer_init_count,
             scs->me_pool_init_count);
    SVT_INFO(
        "SVT [config]: ME_SEG_W0 / ME_SEG_W1 / ME_SEG_W2 / ME_SEG_W3 \t\t\t: %d / %d / %d / %d\n",
        scs->me_segment_column_count_array[0],
        scs->me_segment_column_count_array[1],
        scs->me_segment_column_count_array[2],
        scs->me_segment_column_count_array[3]);
    SVT_INFO(
        "SVT [config]: ME_SEG_H0 / ME_SEG_H1 / ME_SEG_H2 / ME_SEG_H3 \t\t\t: %d / %d / %d / %d\n",
        scs->me_segment_row_count_array[0],
        scs->me_segment_row_count_array[1],
        scs->me_segment_row_count_array[2],
        scs->me_segment_row_count_array[3]);
    SVT_INFO(
        "SVT [config]: ME_SEG_W0 / ME_SEG_W1 / ME_SEG_W2 / ME_SEG_W3 \t\t\t: %d / %d / %d / %d\n",
        scs->enc_dec_segment_col_count_array[0],
        scs->enc_dec_segment_col_count_array[1],
        scs->enc_dec_segment_col_count_array[2],
        scs->enc_dec_segment_col_count_array[3]);
    SVT_INFO(
        "SVT [config]: ME_SEG_H0 / ME_SEG_H1 / ME_SEG_H2 / ME_SEG_H3 \t\t\t: %d / %d / %d / %d\n",
        scs->enc_dec_segment_row_count_array[0],
        scs->enc_dec_segment_row_count_array[1],
        scs->enc_dec_segment_row_count_array[2],
        scs->enc_dec_segment_row_count_array[3]);
    SVT_INFO(
        "SVT [config]: PA_P / ME_P / SBO_P / MDC_P / ED_P / EC_P \t\t\t: %d / %d / %d / %d / %d / "
        "%d\n",
        scs->picture_analysis_process_init_count,
        scs->motion_estimation_process_init_count,
        scs->source_based_operations_process_init_count,
        scs->mode_decision_configuration_process_init_count,
        scs->enc_dec_process_init_count,
        scs->entropy_coding_process_init_count);
    SVT_INFO("SVT [config]: DLF_P / CDEF_P / REST_P \t\t\t\t\t\t: %d / %d / %d\n",
             scs->dlf_process_init_count,
             scs->cdef_process_init_count,
             scs->rest_process_init_count);
#endif
    SVT_INFO("-------------------------------------------\n");

    fflush(stdout);
}

/**********************************
* Parse Single Parameter
**********************************/

static EbErrorType str_to_int64(const char *nptr, int64_t *out, char **nextptr) {
    char   *endptr;
    int64_t val;

    val = strtoll(nptr, &endptr, 0);

    if (endptr == nptr || (!nextptr && *endptr))
        return EB_ErrorBadParameter;

    *out = val;
    if (nextptr)
        *nextptr = endptr;
    return EB_ErrorNone;
}

static EbErrorType str_to_int(const char *nptr, int32_t *out, char **nextptr) {
    char   *endptr;
    int32_t val;

    val = strtol(nptr, &endptr, 0);

    if (endptr == nptr || (!nextptr && *endptr))
        return EB_ErrorBadParameter;

    *out = val;
    if (nextptr)
        *nextptr = endptr;
    return EB_ErrorNone;
}

static EbErrorType str_to_uint64(const char *nptr, uint64_t *out, char **nextptr) {
    char    *endptr;
    uint64_t val;

    if (strtoll(nptr, NULL, 0) < 0) {
        return EB_ErrorBadParameter;
    }

    val = strtoull(nptr, &endptr, 0);

    if (endptr == nptr || (!nextptr && *endptr))
        return EB_ErrorBadParameter;

    *out = val;
    if (nextptr)
        *nextptr = endptr;
    return EB_ErrorNone;
}

static EbErrorType str_to_uint(const char *nptr, uint32_t *out, char **nextptr) {
    char    *endptr;
    uint32_t val;

    if (strtol(nptr, NULL, 0) < 0) {
        return EB_ErrorBadParameter;
    }

    val = strtoul(nptr, &endptr, 0);

    if (endptr == nptr || (!nextptr && *endptr))
        return EB_ErrorBadParameter;

    *out = val;
    if (nextptr)
        *nextptr = endptr;
    return EB_ErrorNone;
}

//assume the input list of values are in the format of "[v1,v2,v3,...]"
static EbErrorType parse_list_s32(const char *nptr, int32_t *list, size_t n) {
    const char *ptr = nptr;
    char       *endptr;
    size_t      i = 0;
    memset(list, 0, n * sizeof(*list));
    while (*ptr) {
        if (*ptr == '[' || *ptr == ']') {
            ptr++;
            continue;
        }

        int32_t     rawval;
        EbErrorType err = str_to_int(ptr, &rawval, &endptr);
        if (err != EB_ErrorNone)
            return err;
        if (i >= n) {
            return EB_ErrorBadParameter;
        } else if (*endptr == ',' || *endptr == ']') {
            endptr++;
        } else if (*endptr) {
            return EB_ErrorBadParameter;
        }
        list[i++] = rawval;
        ptr       = endptr;
    }
    return EB_ErrorNone;
}

static EbErrorType parse_list_u32(const char *nptr, uint32_t *list, size_t n) {
    const char *ptr = nptr;
    char       *endptr;
    size_t      i = 0;
    memset(list, 0, n * sizeof(*list));
    while (*ptr) {
        if (*ptr == '[' || *ptr == ']') {
            ptr++;
            continue;
        }

        uint32_t    rawval;
        EbErrorType err = str_to_uint(ptr, &rawval, &endptr);
        if (err != EB_ErrorNone)
            return err;
        if (i >= n) {
            return EB_ErrorBadParameter;
        } else if (*endptr == ',' || *endptr == ']') {
            endptr++;
        } else if (*endptr) {
            return EB_ErrorBadParameter;
        }
        list[i++] = rawval;
        ptr       = endptr;
    }
    return EB_ErrorNone;
}

static EbErrorType parse_list_u64(const char *nptr, uint64_t *list, size_t n) {
    const char *ptr = nptr;
    char       *endptr;
    size_t      i = 0;
    memset(list, 0, n * sizeof(*list));
    while (*ptr) {
        if (*ptr == '[' || *ptr == ']') {
            ptr++;
            continue;
        }

        uint64_t    rawval;
        EbErrorType err = str_to_uint64(ptr, &rawval, &endptr);
        if (err != EB_ErrorNone)
            return err;
        if (i >= n) {
            return EB_ErrorBadParameter;
        } else if (*endptr == ',' || *endptr == ']') {
            endptr++;
        } else if (*endptr) {
            return EB_ErrorBadParameter;
        }
        list[i++] = rawval;
        ptr       = endptr;
    }
    return EB_ErrorNone;
}

static uint32_t count_params(const char *nptr) {
    const char *ptr = nptr;
    char       *endptr;
    uint32_t    i = 0;
    while (*ptr) {
        if (*ptr == '[' || *ptr == ']') {
            ptr++;
            continue;
        }

        strtoll(ptr, &endptr, 10);
        if (*endptr == ',' || *endptr == ']') {
            endptr++;
        } else if (*endptr) {
            return i;
        }
        i++;
        ptr = endptr;
    }
    return i;
}

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

static EbErrorType str_to_bool(const char *nptr, Bool *out) {
    Bool val;

    if (!strcmp(nptr, "1") || !strcasecmp(nptr, "true") || !strcasecmp(nptr, "yes"))
        val = TRUE;
    else if (!strcmp(nptr, "0") || !strcasecmp(nptr, "false") || !strcasecmp(nptr, "no"))
        val = FALSE;
    else
        return EB_ErrorBadParameter;

    *out = val;
    return EB_ErrorNone;
}

static EbErrorType str_to_crf(const char *nptr, EbSvtAv1EncConfiguration *config_struct) {
    uint32_t    crf;
    EbErrorType return_error;

    return_error = str_to_uint(nptr, &crf, NULL);
    if (return_error == EB_ErrorBadParameter)
        return return_error;

    config_struct->qp                           = crf;
    config_struct->rate_control_mode            = SVT_AV1_RC_MODE_CQP_OR_CRF;
    config_struct->enable_adaptive_quantization = 2;

    return EB_ErrorNone;
}

static EbErrorType str_to_keyint(const char *nptr, int32_t *out, Bool *multi) {
    char      *suff;
    const long keyint = strtol(nptr, &suff, 0);

    if (keyint > INT32_MAX || keyint < -2)
        return EB_ErrorBadParameter;

    switch (*suff) {
    case 's':
        // signal we need to multiply keyint * frame_rate
        *multi = TRUE;
        *out   = keyint;
        break;
    case '\0':
        *multi = FALSE;
        *out   = keyint < 0 ? keyint : keyint - 1;
        break;
    default:
        // else leave as untouched, we have an invalid keyint
        SVT_ERROR("Invalid keyint value: %s\n", nptr);
        return EB_ErrorBadParameter;
    }

    return EB_ErrorNone;
}

static EbErrorType str_to_bitrate(const char *nptr, uint32_t *out) {
    char        *suff;
    const double bitrate = strtod(nptr, &suff);

    if (bitrate < 0 || bitrate > UINT32_MAX) {
        SVT_ERROR("Invalid bitrate value: %s\n", nptr);
        return EB_ErrorBadParameter;
    }

    switch (*suff) {
    case 'b':
    case 'B': *out = (uint32_t)bitrate; break;
    case '\0':
    case 'k':
    case 'K': *out = (uint32_t)(1000 * bitrate); break;
    case 'm':
    case 'M': *out = (uint32_t)(1000000 * bitrate); break;
    default: return EB_ErrorBadParameter;
    }
    if (*out > 100000000) {
        *out = 100000000;
        SVT_WARN("Bitrate value: %s has been set to 100000000\n", nptr);
    }
    return EB_ErrorNone;
}

static EbErrorType str_to_profile(const char *nptr, EbAv1SeqProfile *out) {
    const struct {
        const char     *name;
        EbAv1SeqProfile profile;
    } profiles[] = {
        {"main", MAIN_PROFILE},
        {"high", HIGH_PROFILE},
        {"professional", PROFESSIONAL_PROFILE},
    };
    const size_t profiles_size = sizeof(profiles) / sizeof(profiles[0]);

    for (size_t i = 0; i < profiles_size; i++) {
        if (!strcmp(nptr, profiles[i].name)) {
            *out = profiles[i].profile;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_color_fmt(const char *nptr, EbColorFormat *out) {
    const struct {
        const char   *name;
        EbColorFormat fmt;
    } color_formats[] = {
        {"mono", EB_YUV400},
        {"400", EB_YUV400},
        {"420", EB_YUV420},
        {"422", EB_YUV422},
        {"444", EB_YUV444},
    };
    const size_t color_format_size = sizeof(color_formats) / sizeof(color_formats[0]);

    for (size_t i = 0; i < color_format_size; i++) {
        if (!strcmp(nptr, color_formats[i].name)) {
            *out = color_formats[i].fmt;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_intra_rt(const char *nptr, SvtAv1IntraRefreshType *out) {
    const struct {
        const char            *name;
        SvtAv1IntraRefreshType type;
    } refresh_types[] = {
        {"cra", SVT_AV1_FWDKF_REFRESH},
        {"fwdkf", SVT_AV1_FWDKF_REFRESH},
        {"idr", SVT_AV1_KF_REFRESH},
        {"kf", SVT_AV1_KF_REFRESH},
    };
    const size_t refresh_type_size = sizeof(refresh_types) / sizeof(refresh_types[0]);

    for (size_t i = 0; i < refresh_type_size; i++) {
        if (!strcmp(nptr, refresh_types[i].name)) {
            *out = refresh_types[i].type;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_asm(const char *nptr, EbCpuFlags *out) {
    // need to keep in sync with set_asm_type()
    // need to handle numbers in here since the numbers to no match the
    // internal representation
    const struct {
        const char *name;
        EbCpuFlags  flag;
    } simds[] = {
        {"c", 0},
        {"0", 0},
        {"mmx", (EB_CPU_FLAGS_MMX << 1) - 1},
        {"1", (EB_CPU_FLAGS_MMX << 1) - 1},
        {"sse", (EB_CPU_FLAGS_SSE << 1) - 1},
        {"2", (EB_CPU_FLAGS_SSE << 1) - 1},
        {"sse2", (EB_CPU_FLAGS_SSE2 << 1) - 1},
        {"3", (EB_CPU_FLAGS_SSE2 << 1) - 1},
        {"sse3", (EB_CPU_FLAGS_SSE3 << 1) - 1},
        {"4", (EB_CPU_FLAGS_SSE3 << 1) - 1},
        {"ssse3", (EB_CPU_FLAGS_SSSE3 << 1) - 1},
        {"5", (EB_CPU_FLAGS_SSSE3 << 1) - 1},
        {"sse4_1", (EB_CPU_FLAGS_SSE4_1 << 1) - 1},
        {"6", (EB_CPU_FLAGS_SSE4_1 << 1) - 1},
        {"sse4_2", (EB_CPU_FLAGS_SSE4_2 << 1) - 1},
        {"7", (EB_CPU_FLAGS_SSE4_2 << 1) - 1},
        {"avx", (EB_CPU_FLAGS_AVX << 1) - 1},
        {"8", (EB_CPU_FLAGS_AVX << 1) - 1},
        {"avx2", (EB_CPU_FLAGS_AVX2 << 1) - 1},
        {"9", (EB_CPU_FLAGS_AVX2 << 1) - 1},
        {"avx512", (EB_CPU_FLAGS_AVX512VL << 1) - 1},
        {"10", (EB_CPU_FLAGS_AVX512VL << 1) - 1},
        {"max", EB_CPU_FLAGS_ALL},
        {"11", EB_CPU_FLAGS_ALL},
    };
    const size_t simds_size = sizeof(simds) / sizeof(simds[0]);

    for (size_t i = 0; i < simds_size; i++) {
        if (!strcmp(nptr, simds[i].name)) {
            *out = simds[i].flag;
            return EB_ErrorNone;
        }
    }

    *out = EB_CPU_FLAGS_INVALID;

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_color_primaries(const char *nptr, EbColorPrimaries *out) {
    const struct {
        const char      *name;
        EbColorPrimaries primaries;
    } color_primaries[] = {
        {"bt709", EB_CICP_CP_BT_709},
        {"bt470m", EB_CICP_CP_BT_470_M},
        {"bt470bg", EB_CICP_CP_BT_470_B_G},
        {"bt601", EB_CICP_CP_BT_601},
        {"smpte240", EB_CICP_CP_SMPTE_240},
        {"film", EB_CICP_CP_GENERIC_FILM},
        {"bt2020", EB_CICP_CP_BT_2020},
        {"xyz", EB_CICP_CP_XYZ},
        {"smpte431", EB_CICP_CP_SMPTE_431},
        {"smpte432", EB_CICP_CP_SMPTE_432},
        {"ebu3213", EB_CICP_CP_EBU_3213},
    };
    const size_t color_primaries_size = sizeof(color_primaries) / sizeof(color_primaries[0]);

    for (size_t i = 0; i < color_primaries_size; i++) {
        if (!strcmp(nptr, color_primaries[i].name)) {
            *out = color_primaries[i].primaries;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_transfer_characteristics(const char                *nptr,
                                                   EbTransferCharacteristics *out) {
    const struct {
        const char               *name;
        EbTransferCharacteristics tfc;
    } transfer_characteristics[] = {
        {"bt709", EB_CICP_TC_BT_709},
        {"bt470m", EB_CICP_TC_BT_470_M},
        {"bt470bg", EB_CICP_TC_BT_470_B_G},
        {"bt601", EB_CICP_TC_BT_601},
        {"smpte240", EB_CICP_TC_SMPTE_240},
        {"linear", EB_CICP_TC_LINEAR},
        {"log100", EB_CICP_TC_LOG_100},
        {"log100-sqrt10", EB_CICP_TC_LOG_100_SQRT10},
        {"iec61966", EB_CICP_TC_IEC_61966},
        {"bt1361", EB_CICP_TC_BT_1361},
        {"srgb", EB_CICP_TC_SRGB},
        {"bt2020-10", EB_CICP_TC_BT_2020_10_BIT},
        {"bt2020-12", EB_CICP_TC_BT_2020_12_BIT},
        {"smpte2084", EB_CICP_TC_SMPTE_2084},
        {"smpte428", EB_CICP_TC_SMPTE_428},
        {"hlg", EB_CICP_TC_HLG},
    };
    const size_t transfer_characteristics_size = sizeof(transfer_characteristics) /
        sizeof(transfer_characteristics[0]);

    for (size_t i = 0; i < transfer_characteristics_size; i++) {
        if (!strcmp(nptr, transfer_characteristics[i].name)) {
            *out = transfer_characteristics[i].tfc;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_matrix_coefficients(const char *nptr, EbMatrixCoefficients *out) {
    const struct {
        const char          *name;
        EbMatrixCoefficients coeff;
    } matrix_coefficients[] = {
        {"identity", EB_CICP_MC_IDENTITY},
        {"bt709", EB_CICP_MC_BT_709},
        {"fcc", EB_CICP_MC_FCC},
        {"bt470bg", EB_CICP_MC_BT_470_B_G},
        {"bt601", EB_CICP_MC_BT_601},
        {"smpte240", EB_CICP_MC_SMPTE_240},
        {"ycgco", EB_CICP_MC_SMPTE_YCGCO},
        {"bt2020-ncl", EB_CICP_MC_BT_2020_NCL},
        {"bt2020-cl", EB_CICP_MC_BT_2020_CL},
        {"smpte2085", EB_CICP_MC_SMPTE_2085},
        {"chroma-ncl", EB_CICP_MC_CHROMAT_NCL},
        {"chroma-cl", EB_CICP_MC_CHROMAT_CL},
        {"ictcp", EB_CICP_MC_ICTCP},
    };
    const size_t matrix_coefficients_size = sizeof(matrix_coefficients) /
        sizeof(matrix_coefficients[0]);

    for (size_t i = 0; i < matrix_coefficients_size; i++) {
        if (!strcmp(nptr, matrix_coefficients[i].name)) {
            *out = matrix_coefficients[i].coeff;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_color_range(const char *nptr, EbColorRange *out) {
    const struct {
        const char  *name;
        EbColorRange range;
    } color_range[] = {
        {"studio", EB_CR_STUDIO_RANGE},
        {"full", EB_CR_FULL_RANGE},
    };
    const size_t color_range_size = sizeof(color_range) / sizeof(color_range[0]);

    for (size_t i = 0; i < color_range_size; i++) {
        if (!strcmp(nptr, color_range[i].name)) {
            *out = color_range[i].range;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_chroma_sample_position(const char *nptr, EbChromaSamplePosition *out) {
    const struct {
        const char            *name;
        EbChromaSamplePosition pos;
    } chroma_sample_positions[] = {
        {"unknown", EB_CSP_UNKNOWN},
        {"vertical", EB_CSP_VERTICAL},
        {"left", EB_CSP_VERTICAL},
        {"colocated", EB_CSP_COLOCATED},
        {"topleft", EB_CSP_COLOCATED},
    };
    const size_t chroma_sample_positions_size = sizeof(chroma_sample_positions) /
        sizeof(chroma_sample_positions[0]);

    for (size_t i = 0; i < chroma_sample_positions_size; i++) {
        if (!strcmp(nptr, chroma_sample_positions[i].name)) {
            *out = chroma_sample_positions[i].pos;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_sframe_mode(const char *nptr, EbSFrameMode *out) {
    const struct {
        const char  *name;
        EbSFrameMode mode;
    } sframe_mode[] = {
        {"strict", SFRAME_STRICT_BASE},
        {"nearest", SFRAME_NEAREST_BASE},
    };
    const size_t sframe_mode_size = sizeof(sframe_mode) / sizeof(sframe_mode[0]);

    for (size_t i = 0; i < sframe_mode_size; i++) {
        if (!strcmp(nptr, sframe_mode[i].name)) {
            *out = sframe_mode[i].mode;
            return EB_ErrorNone;
        }
    }

    return EB_ErrorBadParameter;
}

static EbErrorType str_to_rc_mode(const char *nptr, uint32_t *out, uint8_t *aq_mode,
                                  uint8_t *pred_structure) {
    // separate rc mode enum to distinguish between cqp and crf modes
    enum rc_modes {
        RC_MODE_ZERO = 0, // unique mode in case user passes a literal 0
        RC_MODE_CQP,
        RC_MODE_CRF,
        RC_MODE_VBR,
        RC_MODE_CBR,
        RC_MODE_INVALID,
    };
    const struct {
        const char *name;
        uint32_t    mode;
    } rc_mode[] = {
        {"0", RC_MODE_ZERO},
        {"1", RC_MODE_VBR},
        {"2", RC_MODE_CBR},
        {"cqp", RC_MODE_CQP},
        {"crf", RC_MODE_CRF},
        {"vbr", RC_MODE_VBR},
        {"cbr", RC_MODE_CBR},
    };
    const size_t rc_mode_size = sizeof(rc_mode) / sizeof(rc_mode[0]);

    enum rc_modes mode = RC_MODE_INVALID;

    for (size_t i = 0; i < rc_mode_size; i++) {
        if (!strcmp(nptr, rc_mode[i].name)) {
            mode = rc_mode[i].mode;
            break;
        }
    }

    switch (mode) {
    case RC_MODE_ZERO: *out = 0; break;
    case RC_MODE_CQP:
        *out     = SVT_AV1_RC_MODE_CQP_OR_CRF;
        *aq_mode = 0;
        break;
    case RC_MODE_CRF:
        *out     = SVT_AV1_RC_MODE_CQP_OR_CRF;
        *aq_mode = 2;
        break;
    case RC_MODE_VBR: *out = SVT_AV1_RC_MODE_VBR; break;
    case RC_MODE_CBR:
        *out            = SVT_AV1_RC_MODE_CBR;
        *pred_structure = SVT_AV1_PRED_LOW_DELAY_B;
        break;
    default: SVT_ERROR("Invalid rc mode: %s\n", nptr); return EB_ErrorBadParameter;
    }
    return EB_ErrorNone;
}

static EbErrorType str_to_frm_resz_evts(const char *nptr, SvtAv1FrameScaleEvts *evts) {
    const uint32_t param_count = count_params(nptr);
    if ((evts->evt_num != 0 && evts->evt_num != param_count) || param_count == 0) {
        SVT_ERROR("Error: Size for the list passed to %s doesn't match %u\n",
                  "frame-resz-events",
                  evts->evt_num);
        return EB_ErrorBadParameter;
    }
    if (evts->start_frame_nums)
        EB_FREE(evts->start_frame_nums);
    EB_MALLOC(evts->start_frame_nums, param_count * sizeof(uint64_t));
    evts->evt_num = param_count;
    return parse_list_u64(nptr, evts->start_frame_nums, param_count);
}

static EbErrorType str_to_resz_kf_denoms(const char *nptr, SvtAv1FrameScaleEvts *evts) {
    const uint32_t param_count = count_params(nptr);
    if ((evts->evt_num != 0 && evts->evt_num != param_count) || param_count == 0) {
        SVT_ERROR("Error: Size for the list passed to %s doesn't match %u\n",
                  "frame-resz-kf-denoms",
                  evts->evt_num);
        return EB_ErrorBadParameter;
    }
    if (evts->resize_kf_denoms)
        EB_FREE(evts->resize_kf_denoms);
    EB_MALLOC(evts->resize_kf_denoms, param_count * sizeof(uint32_t));
    evts->evt_num = param_count;
    return parse_list_u32(nptr, evts->resize_kf_denoms, param_count);
}

static EbErrorType str_to_resz_denoms(const char *nptr, SvtAv1FrameScaleEvts *evts) {
    const uint32_t param_count = count_params(nptr);
    if ((evts->evt_num != 0 && evts->evt_num != param_count) || param_count == 0) {
        SVT_ERROR("Error: Size for the list passed to %s doesn't match %u\n",
                  "frame-resz-denoms",
                  evts->evt_num);
        return EB_ErrorBadParameter;
    }
    if (evts->resize_denoms)
        EB_FREE(evts->resize_denoms);
    EB_MALLOC(evts->resize_denoms, param_count * sizeof(uint32_t));
    evts->evt_num = param_count;
    return parse_list_u32(nptr, evts->resize_denoms, param_count);
}

#define COLOR_OPT(par, opt)                                          \
    do {                                                             \
        if (!strcmp(name, par)) {                                    \
            return_error = str_to_##opt(value, &config_struct->opt); \
            if (return_error == EB_ErrorNone)                        \
                return return_error;                                 \
            uint32_t val;                                            \
            return_error = str_to_uint(value, &val, NULL);           \
            if (return_error == EB_ErrorNone)                        \
                config_struct->opt = val;                            \
            return return_error;                                     \
        }                                                            \
    } while (0)

#define COLOR_METADATA_OPT(par, opt)                                                       \
    do {                                                                                   \
        if (!strcmp(name, par))                                                            \
            return svt_aom_parse_##opt(&config_struct->opt, value) ? EB_ErrorNone          \
                                                                   : EB_ErrorBadParameter; \
    } while (0)

EB_API EbErrorType svt_av1_enc_parse_parameter(EbSvtAv1EncConfiguration *config_struct,
                                               const char *name, const char *value) {
    if (config_struct == NULL || name == NULL || value == NULL)
        return EB_ErrorBadParameter;

    EbErrorType return_error = EB_ErrorBadParameter;

    if (!strcmp(name, "keyint"))
        return str_to_keyint(
            value, &config_struct->intra_period_length, &config_struct->multiply_keyint);

    if (!strcmp(name, "tbr"))
        return str_to_bitrate(value, &config_struct->target_bit_rate);

    if (!strcmp(name, "mbr"))
        return str_to_bitrate(value, &config_struct->max_bit_rate);

    // options updating more than one field
    if (!strcmp(name, "crf"))
        return str_to_crf(value, config_struct);

    if (!strcmp(name, "rc"))
        return str_to_rc_mode(value,
                              &config_struct->rate_control_mode,
                              &config_struct->enable_adaptive_quantization,
                              &config_struct->pred_structure);

    // custom enum fields
    if (!strcmp(name, "profile"))
        return str_to_profile(value, &config_struct->profile) == EB_ErrorBadParameter
            ? str_to_uint(value, (uint32_t *)&config_struct->profile, NULL)
            : EB_ErrorNone;

    if (!strcmp(name, "color-format"))
        return str_to_color_fmt(value, &config_struct->encoder_color_format) == EB_ErrorBadParameter
            ? str_to_uint(value, (uint32_t *)&config_struct->encoder_color_format, NULL)
            : EB_ErrorNone;

    if (!strcmp(name, "irefresh-type"))
        return str_to_intra_rt(value, &config_struct->intra_refresh_type) == EB_ErrorBadParameter
            ? str_to_uint(value, (uint32_t *)&config_struct->intra_refresh_type, NULL)
            : EB_ErrorNone;

    if (!strcmp(name, "sframe-mode"))
        return str_to_sframe_mode(value, &config_struct->sframe_mode) == EB_ErrorBadParameter
            ? str_to_uint(value, (uint32_t *)&config_struct->sframe_mode, NULL)
            : EB_ErrorNone;

    if (!strcmp(name, "asm"))
        return str_to_asm(value, &config_struct->use_cpu_flags);

    COLOR_OPT("color-primaries", color_primaries);
    COLOR_OPT("transfer-characteristics", transfer_characteristics);
    COLOR_OPT("matrix-coefficients", matrix_coefficients);
    COLOR_OPT("color-range", color_range);
    COLOR_OPT("chroma-sample-position", chroma_sample_position);

    // custom struct fields
    COLOR_METADATA_OPT("mastering-display", mastering_display);
    COLOR_METADATA_OPT("content-light", content_light_level);

    // arrays
    if (!strcmp(name, "qindex-offsets"))
        return parse_list_s32(value, config_struct->qindex_offsets, EB_MAX_TEMPORAL_LAYERS);

    if (!strcmp(name, "chroma-qindex-offsets"))
        return parse_list_s32(value, config_struct->chroma_qindex_offsets, EB_MAX_TEMPORAL_LAYERS);

    if (!strcmp(name, "lambda-scale-factors"))
        return parse_list_s32(
            value, config_struct->lambda_scale_factors, SVT_AV1_FRAME_UPDATE_TYPES);

    if (!strcmp(name, "frame-resz-events"))
        return str_to_frm_resz_evts(value, &config_struct->frame_scale_evts);

    if (!strcmp(name, "frame-resz-kf-denoms"))
        return str_to_resz_kf_denoms(value, &config_struct->frame_scale_evts);

    if (!strcmp(name, "frame-resz-denoms"))
        return str_to_resz_denoms(value, &config_struct->frame_scale_evts);

    // uint32_t fields
    const struct {
        const char *name;
        uint32_t   *out;
    } uint_opts[] = {
        {"w", &config_struct->source_width},
        {"width", &config_struct->source_width},
        {"h", &config_struct->source_height},
        {"height", &config_struct->source_height},
        {"q", &config_struct->qp},
        {"qp", &config_struct->qp},
        {"film-grain", &config_struct->film_grain_denoise_strength},
        {"hierarchical-levels", &config_struct->hierarchical_levels},
        {"tier", &config_struct->tier},
        {"level", &config_struct->level},
        {"lp", &config_struct->logical_processors},
        {"pin", &config_struct->pin_threads},
        {"fps-num", &config_struct->frame_rate_numerator},
        {"fps-denom", &config_struct->frame_rate_denominator},
        {"lookahead", &config_struct->look_ahead_distance},
        {"scd", &config_struct->scene_change_detection},
        {"max-qp", &config_struct->max_qp_allowed},
        {"min-qp", &config_struct->min_qp_allowed},
        {"bias-pct", &config_struct->vbr_bias_pct},
        {"minsection-pct", &config_struct->vbr_min_section_pct},
        {"maxsection-pct", &config_struct->vbr_max_section_pct},
        {"undershoot-pct", &config_struct->under_shoot_pct},
        {"overshoot-pct", &config_struct->over_shoot_pct},
        {"mbr-overshoot-pct", &config_struct->mbr_over_shoot_pct},
        {"recode-loop", &config_struct->recode_loop},
        {"enable-stat-report", &config_struct->stat_report},
        {"scm", &config_struct->screen_content_mode},
        {"input-depth", &config_struct->encoder_bit_depth},
        {"compressed-ten-bit-format", &config_struct->compressed_ten_bit_format},
        {"forced-max-frame-width", &config_struct->forced_max_frame_width},
        {"forced-max-frame-height", &config_struct->forced_max_frame_height},
    };
    const size_t uint_opts_size = sizeof(uint_opts) / sizeof(uint_opts[0]);

    for (size_t i = 0; i < uint_opts_size; i++) {
        if (!strcmp(name, uint_opts[i].name)) {
            return str_to_uint(value, uint_opts[i].out, NULL);
        }
    }

    // uint8_t fields
    const struct {
        const char *name;
        uint8_t    *out;
    } uint8_opts[] = {
        {"pred-struct", &config_struct->pred_structure},
        {"enable-tpl-la", &config_struct->enable_tpl_la},
        {"aq-mode", &config_struct->enable_adaptive_quantization},
        {"superres-mode", &config_struct->superres_mode},
        {"superres-qthres", &config_struct->superres_qthres},
        {"superres-kf-qthres", &config_struct->superres_kf_qthres},
        {"superres-denom", &config_struct->superres_denom},
        {"superres-kf-denom", &config_struct->superres_kf_denom},
        {"tune", &config_struct->tune},
        {"film-grain-denoise", &config_struct->film_grain_denoise_apply},
        {"enable-hdr", &config_struct->high_dynamic_range_input},
        {"resize-mode", &config_struct->resize_mode},
        {"resize-denom", &config_struct->resize_denom},
        {"resize-kf-denom", &config_struct->resize_kf_denom},
        {"qm-min", &config_struct->min_qm_level},
        {"qm-max", &config_struct->max_qm_level},
        {"use-fixed-qindex-offsets", &config_struct->use_fixed_qindex_offsets},
#if FTR_STARTUP_MG_SIZE
        {"startup-mg-size", &config_struct->startup_mg_size},
#endif
    };
    const size_t uint8_opts_size = sizeof(uint8_opts) / sizeof(uint8_opts[0]);

    for (size_t i = 0; i < uint8_opts_size; i++) {
        if (!strcmp(name, uint8_opts[i].name)) {
            uint32_t val;
            return_error = str_to_uint(value, &val, NULL);
            if (return_error == EB_ErrorNone)
                *uint8_opts[i].out = val;
            return return_error;
        }
    }

    // int64_t fields
    const struct {
        const char *name;
        int64_t    *out;
    } int64_opts[] = {
        {"buf-initial-sz", &config_struct->starting_buffer_level_ms},
        {"buf-optimal-sz", &config_struct->optimal_buffer_level_ms},
        {"buf-sz", &config_struct->maximum_buffer_size_ms},
    };
    const size_t int64_opts_size = sizeof(int64_opts) / sizeof(int64_opts[0]);

    for (size_t i = 0; i < int64_opts_size; i++) {
        if (!strcmp(name, int64_opts[i].name)) {
            return str_to_int64(value, int64_opts[i].out, NULL);
        }
    }

    // int32_t fields
    const struct {
        const char *name;
        int32_t    *out;
    } int_opts[] = {
        {"key-frame-chroma-qindex-offset", &config_struct->key_frame_chroma_qindex_offset},
        {"key-frame-qindex-offset", &config_struct->key_frame_qindex_offset},
        {"luma-y-dc-qindex-offset", &config_struct->luma_y_dc_qindex_offset},
        {"chroma-u-dc-qindex-offset", &config_struct->chroma_u_dc_qindex_offset},
        {"chroma-u-ac-qindex-offset", &config_struct->chroma_u_ac_qindex_offset},
        {"chroma-v-dc-qindex-offset", &config_struct->chroma_v_dc_qindex_offset},
        {"chroma-v-ac-qindex-offset", &config_struct->chroma_v_ac_qindex_offset},
        {"pass", &config_struct->pass},
        {"enable-cdef", &config_struct->cdef_level},
        {"enable-restoration", &config_struct->enable_restoration_filtering},
        {"enable-mfmv", &config_struct->enable_mfmv},
        {"intra-period", &config_struct->intra_period_length},
        {"tile-rows", &config_struct->tile_rows},
        {"tile-columns", &config_struct->tile_columns},
        {"ss", &config_struct->target_socket},
        {"sframe-dist", &config_struct->sframe_dist},
    };
    const size_t int_opts_size = sizeof(int_opts) / sizeof(int_opts[0]);

    for (size_t i = 0; i < int_opts_size; i++) {
        if (!strcmp(name, int_opts[i].name)) {
            return str_to_int(value, int_opts[i].out, NULL);
        }
    }

    // int8_t fields
    const struct {
        const char *name;
        int8_t     *out;
    } int8_opts[] = {
        {"preset", &config_struct->enc_mode},
    };
    const size_t int8_opts_size = sizeof(int8_opts) / sizeof(int8_opts[0]);

    for (size_t i = 0; i < int8_opts_size; i++) {
        if (!strcmp(name, int8_opts[i].name)) {
            int32_t val;
            return_error = str_to_int(value, &val, NULL);
            if (return_error == EB_ErrorNone)
                *int8_opts[i].out = val;
            return return_error;
        }
    }

    // Bool fields
    const struct {
        const char *name;
        Bool       *out;
    } bool_opts[] = {
        {"use-q-file", &config_struct->use_qp_file},
        {"enable-dlf", &config_struct->enable_dlf_flag},
        {"rmv", &config_struct->restricted_motion_vector},
        {"enable-tf", &config_struct->enable_tf},
        {"enable-overlays", &config_struct->enable_overlays},
        {"fast-decode", &config_struct->fast_decode},
        {"enable-force-key-frames", &config_struct->force_key_frames},
        {"enable-qm", &config_struct->enable_qm},
        {"enable-dg", &config_struct->enable_dg},
        {"gop-constraint-rc", &config_struct->gop_constraint_rc},
    };
    const size_t bool_opts_size = sizeof(bool_opts) / sizeof(bool_opts[0]);

    for (size_t i = 0; i < bool_opts_size; i++) {
        if (!strcmp(name, bool_opts[i].name)) {
            return str_to_bool(value, bool_opts[i].out);
        }
    }

    return return_error;
}
