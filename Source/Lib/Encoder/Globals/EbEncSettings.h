/*
* Copyright(c) 2022 Intel Corporation
*
* This source code is subject to the terms of the BSD 2 Clause License and
* the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
* was not distributed with this source code in the LICENSE file, you can
* obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
* Media Patent License 1.0 was not distributed with this source code in the
* PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
*/

#ifndef EbEncSettings_h
#define EbEncSettings_h

#include "EbSvtAv1Enc.h"
#include "EbDebugMacros.h"
#include "EbSequenceControlSet.h"

/**************************************
 * Defines
 **************************************/
#define DEFAULT_QP 35

struct SvtAv1PrivOptions {
    /* This structure is meant to allow for the extension and shortening of features.
    * This allows library and end users to use these capabilities through svt_av1_enc_parse_parameter without requiring a wrapper extension.
    * New feature applications would no longer need recompilation and relinking. */

    /* Dynamic gop
    *
    * 0 = disable Dynamic GoP
    * 1 = enable Dynamic GoP
    *  Default is 1. */
    Bool enable_dg;

#if FTR_STARTUP_MG_SIZE
    /**
     * @brief startup_mg_size
     *
     * When enabled, a MG with specified size will be inserted after the key frame.
     * The MG size is determined by 2^startup_mg_size.
     *
     * 0: off
     * 2: set hierarchical levels to 2 (MG size 4)
     * 3: set hierarchical levels to 3 (MG size 8)
     * 4: set hierarchical levels to 4 (MG size 16)
     * Default is 0.
     */
    uint8_t startup_mg_size;
#endif

    /**
     * @brief OverShoot Percentage
     *
     * Only applicable for VBR and CBR
     *
     * Indicates the tolerance of the VBR algorithm to overshoot and is used as
     * a trigger threshold for more agressive adaptation of Quantization.
     *
     * Min is 0.
     * Max is 100.
     * Default is 25.
     */
    uint32_t over_shoot_pct;

    // Individual tuning flags
    /* use fixed qp offset for every picture based on temporal layer index
    * 0: off (use the auto mode QP)
    * 1: on (the offset is applied on top of the user QP)
    * 2: on (the offset is applied on top of the auto mode QP)
    *
    * Default is 0.*/
    uint8_t use_fixed_qindex_offsets;
    int32_t qindex_offsets[EB_MAX_TEMPORAL_LAYERS];
    int32_t key_frame_chroma_qindex_offset;
    int32_t key_frame_qindex_offset;
    int32_t chroma_qindex_offsets[EB_MAX_TEMPORAL_LAYERS];

    int32_t luma_y_dc_qindex_offset;
    int32_t chroma_u_dc_qindex_offset;
    int32_t chroma_u_ac_qindex_offset;
    int32_t chroma_v_dc_qindex_offset;
    int32_t chroma_v_ac_qindex_offset;

    /**
    * @brief Determines how much denoising is used.
    * Only applicable when film grain is ON.
    *
    * 0 is no denoising
    * 1 is full denoising
    */
    uint8_t film_grain_denoise_apply;

    /**
     * @brief Starting Buffer Level in MilliSeconds
     *
     * Only applicable for CBR.
     *
     * Indicates the amount of data that will be buffered by the decoding
     * application prior to beginning playback, and is expressed in units of
     * time. Must be less than maximum_buffer_size_ms.
     *
     * Min is 20.
     * Max is 10000.
     * Default is 600.
     */
    int64_t starting_buffer_level_ms;

    /**
     * @brief Variable Bit Rate Bias Percentage
     *
     * Indicates the bias for determining target size for the current frame.
     * A value 0 indicates the optimal CBR mode value should be used, and 100
     * indicates the optimal VBR mode value should be used.
     *
     * Min is 0.
     * Max is 100.
     * Default is 100
     */
    uint32_t vbr_bias_pct;

    /* Enable TPL in look ahead
     * 0 = disable TPL in look ahead
     * 1 = enable TPL in look ahead
     * Default is 0  */
    uint8_t enable_tpl_la;

    /**
     * @brief Signal to the library to enable quantisation matrices
     *
     * Default is false.
     */
    Bool enable_qm;

#if !SVT_AV1_CHECK_VERSION(1, 5, 0)
    /* DEPRECATED: to be removed in 1.5.0. */
    PredictionStructureConfigEntry pred_struct[1 << (MAX_HIERARCHICAL_LEVEL - 1)];

    /* DEPRECATED: to be removed in 1.5.0. */
    Bool enable_manual_pred_struct;

    /* DEPRECATED: to be removed in 1.5.0. */
    int32_t manual_pred_struct_entry_num;
#endif

    /**
     * @brief Tune for a particular metric; 0: VQ, 1: PSNR
     *
     * Default is 1.
     */
    uint8_t tune;

    // super-resolution parameters
    uint8_t superres_mode;
    uint8_t superres_denom;
    uint8_t superres_kf_denom;
    uint8_t superres_qthres;
    uint8_t superres_kf_qthres;
    uint8_t superres_auto_search_type;

    /**
     * @brief Min quant matrix flatness. Applicable when enable_qm is true.
     * Min value is 0.
     * Max value is 15.
     * Default is 8.
     */
    uint8_t min_qm_level;

    /* Color description present flag
    *
    * It is not necessary to set this parameter manually.
    * It is set internally to true once one of the color_primaries, transfer_characteristics or
    * matrix coefficients is set to non-default value.
    *
    Default is false. */
    Bool color_description_present_flag;

    /* Prediction structure used to construct GOP. There are two main structures
     * supported, which are: Low Delay (P or B) and Random Access.
     *
     * In Low Delay structure, pictures within a mini GOP refer to the previously
     * encoded pictures in display order. In other words, pictures with display
     * order N can only be referenced by pictures with display order greater than
     * N, and it can only refer pictures with picture order lower than N. The Low
     * Delay structure can be flat structured (e.g. IPPPPPPP...) or hierarchically
     * structured. B/b pictures can be used instead of P/p pictures. However, the
     * reference picture list 0 and the reference picture list 1 will contain the
     * same reference picture.
     *
     * In Random Access structure, the B/b pictures can refer to reference pictures
     * from both directions (past and future).
     *
     * Refer to SvtAv1PredStructure enum for valid values.
     *
     * Default is SVT_AV1_PRED_RANDOM_ACCESS. */
    uint8_t pred_structure;

    /**
     * @brief Optimal Buffer Level in MilliSeconds
     *
     * Only applicable for CBR.
     *
     * indicates the amount of data that the encoder should try to maintain in the
     * decoder's buffer, and is expressed in units of time. Must be less than
     * maximum_buffer_size_ms.
     *
     * Min is 20.
     * Max is 10000.
     * Default is 600.
     */
    int64_t optimal_buffer_level_ms;

    /* motion field motion vector
    *
    *  Default is -1. */
    int enable_mfmv;

    /**
     * @brief UnderShoot Percentage
     *
     * Only applicable for VBR and CBR.
     *
     * Indicates the tolerance of the VBR algorithm to undershoot and is used
     * as a trigger threshold for more agressive adaptation of Quantization.
     *
     * Min is 0.
     * Max is 100.
     * Default is 25.
     */
    uint32_t under_shoot_pct;

    /* @brief reference scaling events for random access mode (reize-mode = 4)
     *
     * evt_num:          total count of events
     * start_frame_nums: array of scaling start frame numbers
     * resize_kf_denoms: array of scaling denominators of key-frame
     * resize_denoms:    array of scaling denominators of non-key-frame
     */
    SvtAv1FrameScaleEvts frame_scale_evts;

    /* Restoration filtering
    *  enable/disable
    *  set Self-Guided (sg) mode
    *  set Wiener (wn) mode
    *
    * Default is -1. */
    int enable_restoration_filtering;

    /**
     * @brief Variable Bit Rate Maximum Section Percentage
     *
     * Indicates the maximum bitrate to be used for a single GOP as a percentage
     * of the target bitrate.
     *
     * Min is 0.
     * Max is 10000.
     * Default is 2000.
     */
    uint32_t vbr_max_section_pct;

    /* Decoder-speed-targeted encoder optimization level (produce bitstreams that can be decoded faster).
    * 0: No decoder speed optimization
    * 1: Decoder speed optimization enabled (fast decode)
    */
    Bool fast_decode;

    /* Film grain denoising the input picture
    * Flag to enable the denoising
    *
    * Default is 0. */
    uint32_t film_grain_denoise_strength;

    /**
     * @brief gop_constraint_rc
     *
     * Currently, only applicable for VBR and  when GoP size is greater than 119 frames.
     *
     * When enabled, the rate control matches the target rate for each GoP.
     *
     * 0: off
     * 1: on
     * Default is 0.
     */
    Bool gop_constraint_rc;

    /* Flag to signal the content being a screen sharing content type
    *
    * Default is 0. */
    uint32_t screen_content_mode;

    /* Specifies the maximum frame width/height for the frames represented by the sequence header
     * (max_frame_width_minus_1 and max_frame_height_minus_1, spec 5.5.1).
     * Actual frame height could be equal to or less than this value. E.g. Use this value to indicate
     * the maximum height between renditions when switch frame feature is on.
     */
    uint32_t forced_max_frame_width;
    uint32_t forced_max_frame_height;

#if !SVT_AV1_CHECK_VERSION(1, 5, 0)
    /* DEPRECATED: to be removed in 1.5.0. */
    uint32_t vbv_bufsize;
#endif

    /* recode_loop indicates the recode levels,
     * DISALLOW_RECODE = 0, No recode.
     * ALLOW_RECODE_KFMAXBW = 1, Allow recode for KF and exceeding maximum frame bandwidth.
     * ALLOW_RECODE_KFARFGF = 2, Allow recode only for KF/ARF/GF frames.
     * ALLOW_RECODE = 3, Allow recode for all frames based on bitrate constraints.
     * ALLOW_RECODE_DEFAULT = 4, Default setting, ALLOW_RECODE_KFARFGF for M0~5 and
     *                                            ALLOW_RECODE_KFMAXBW for M6~8.
     * default is 4
     */
    uint32_t recode_loop;

    /**
     * @brief API signal to constrain motion vectors.
     *
     * Default is false.
     */
    Bool restricted_motion_vector;

    /**
     * @brief Resize denominator
     * this value can be from 8 to 16, means downscaling to 8/8-8/16 of original
     * resolution in both width and height
     */
    uint8_t resize_denom;

    /**
     * @brief Enable use of ALT-REF (temporally filtered) frames.
     *
     * Default is true.
     */
    Bool enable_tf;

    Bool enable_overlays;

    /**
     * @brief MaxBitRate OverShoot Percentage
     *
     * Only applicable for Capped CRF.
     *
     * Indicates the tolerance of the Capped CRF algorithm to overshoot
     * and is used as a trigger threshold for more agressive adaptation of
     * Quantization.
     *
     * Min is 0.
     * Max is 100.
     * Default is 50.
     */
    uint32_t mbr_over_shoot_pct;

    /* Unpin the execution .This option does not
    * set the execution to be pinned to a specific number of cores when set to 1. this allows the execution
    * of multiple encodes on the CPU without having to pin them to a specific mask
    * 1: pinned threads
    * 0: unpinned
    * default 0 */
    uint32_t pin_threads;

    /**
     * @brief Max quant matrix flatness. Applicable when enable_qm is true.
     * Min value is 0.
     * Max value is 15.
     * Default is 15.
     */
    uint8_t max_qm_level;

    /**
     * @brief Currently unused.
     *
     * Default is 0.
     */
    uint8_t high_dynamic_range_input;

    /**
     * @brief Resize denominator of key frames
     * this value can be from 8 to 16, means downscaling to 8/8-8/16 of original
     * resolution in both width and height
     */
    uint8_t resize_kf_denom;

    // reference scaling parameters
    /**
     * @brief Reference scaling mode
     * the available modes are defined in RESIZE_MODE
     */
    uint8_t resize_mode;

    /* Indicates how an S-Frame should be inserted
    * values are from EbSFrameMode
    * SFRAME_STRICT_ARF: the considered frame will be made into an S-Frame only if it is an altref frame
    * SFRAME_NEAREST_ARF: if the considered frame is not an altref frame, the next altref frame will be made into an S-Frame
    */
    EbSFrameMode sframe_mode;

    /**
     * @brief Variable Bit Rate Minimum Section Percentage
     *
     * Indicates the minimum bitrate to be used for a single GOP as a percentage
     * of the target bitrate.
     *
     * Min is 0.
     * Max is 100.
     * Default is 0.
     */
    uint32_t vbr_min_section_pct;

    // Threads management

    /* The number of logical processor which encoder threads run on. If
     * LogicalProcessors and TargetSocket are not set, threads are managed by
     * OS thread scheduler. */
    uint32_t logical_processors;

    /* CPU FLAGS to limit assembly instruction set used by encoder.
    * Default is EB_CPU_FLAGS_ALL. */
    EbCpuFlags use_cpu_flags;

    /**
     * @brief Deblocking loop filter control
     *
     * Default is true.
     */
    Bool enable_dlf_flag;

    /**
     * @brief scale factors for lambda value for different frame update types
     * factor >> 7 (/ 128) is the actual value in float
     */
    int32_t lambda_scale_factors[SVT_AV1_FRAME_UPDATE_TYPES];

    /* S-Frame interval (frames)
    * 0: S-Frame off
    * >0: S-Frame on and indicates the number of frames after which a frame may be coded as an S-Frame
    */
    int32_t sframe_dist;
};
struct SvtAv1PrivOptions;

void svt_av1_print_lib_params(SequenceControlSet* scs);

EbErrorType svt_av1_verify_settings(struct SequenceControlSet* scs);

EbErrorType svt_av1_set_default_params(EbSvtAv1EncConfiguration* config_ptr);

#endif // EbEncSettings_h
