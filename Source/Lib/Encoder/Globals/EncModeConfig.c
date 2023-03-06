// Get the nsq_level used for each preset (to be passed to setting function: set_nsq_ctrls())
uint8_t svt_aom_get_nsq_level(EncMode enc_mode, uint8_t is_islice, uint8_t is_base,
                              InputCoeffLvl coeff_lvl) {
    uint8_t nsq_level;
    // set the nsq_level
    if (enc_mode <= ENC_MRS)
        nsq_level = 1;
    else if (enc_mode <= ENC_MR)
        nsq_level = is_islice ? 2 : 3;
    else if (enc_mode <= ENC_M0)
        nsq_level = is_islice ? 2 : 4;
    else if (enc_mode <= ENC_M1)
        nsq_level = is_base ? 5 : 6;
    else if (enc_mode <= ENC_M3)
        nsq_level = is_base ? 7 : 8;
    else if (enc_mode <= ENC_M4) {
        if (coeff_lvl == LOW_LVL)
            nsq_level = is_base ? 9 : 12;
        else if (coeff_lvl == HIGH_LVL)
            nsq_level = is_base ? 11 : 14;
        else // regular
            nsq_level = is_base ? 10 : 13;
    } else if (enc_mode <= ENC_M5) {
        if (coeff_lvl == LOW_LVL)
            nsq_level = is_base ? 15 : 0;
        else if (coeff_lvl == HIGH_LVL)
            nsq_level = is_base ? 17 : 0;
        else // regular
            nsq_level = is_base ? 16 : 0;
    } else
        nsq_level = 0;

    return nsq_level;
}


/*
* return the 4x4 level
Used by svt_aom_signal_derivation_enc_dec_kernel_oq and memory allocation
*/
bool svt_aom_get_disallow_4x4(EncMode enc_mode, SliceType slice_type) {
    (void)slice_type;
    if (enc_mode <= ENC_M5)
        return false;
    else
        return true;
}

// Returns the level for Wiener restoration filter
uint8_t svt_aom_get_wn_filter_level(EncMode enc_mode, uint8_t input_resolution, Bool is_ref) {
    uint8_t wn_filter_lvl = 0;
    if (enc_mode <= ENC_M4)
        wn_filter_lvl = 1;
    else if (enc_mode <= ENC_M8)
        wn_filter_lvl = is_ref ? 5 : 0;
    else
        wn_filter_lvl = 0;

    // higher resolutions will shut restoration to save memory
    if (input_resolution >= INPUT_SIZE_8K_RANGE)
        wn_filter_lvl = 0;

    return wn_filter_lvl;
}

// Returns the level for self-guided restoration filter
uint8_t svt_aom_get_sg_filter_level(EncMode enc_mode, Bool fast_decode, uint8_t input_resolution, Bool is_base) {
    uint8_t sg_filter_lvl = 0;
    if (fast_decode == 0) {
        if (enc_mode <= ENC_M2)
            sg_filter_lvl = 1;
        else if (enc_mode <= ENC_M4)
            sg_filter_lvl = is_base ? 1 : 4;
        else
            sg_filter_lvl = 0;
    }
    else {
        if (enc_mode <= ENC_M2)
            sg_filter_lvl = input_resolution <= INPUT_SIZE_360p_RANGE ? 1 : 0;
        else if (enc_mode <= ENC_M4)
            sg_filter_lvl = input_resolution <= INPUT_SIZE_360p_RANGE ? (is_base ? 1 : 4)
                                                                       : 0;
        else if (enc_mode <= ENC_M5)
            sg_filter_lvl = input_resolution <= INPUT_SIZE_360p_RANGE ? (is_base ? 1 : 0) : 0;
        else
            sg_filter_lvl = 0;
    }

    // higher resolutions will shut restoration to save memory
    if (input_resolution >= INPUT_SIZE_8K_RANGE)
        sg_filter_lvl = 0;

    return sg_filter_lvl;
}

/*
* return the DLF level
* Used by signal_derivation_multi_processes_oq and memory allocation
*
* 0 -- DLF OFF
* 1 -- Full Frame DLF
* 2 -- SB-Based DLF
* 3 -- SB-Based DLF + skip DLF if SB has 0 coeffs
*/
#if OPT_LD_M9
static uint8_t get_dlf_level(EncMode enc_mode, uint8_t is_used_as_reference_flag, uint8_t is_16bit, Bool fast_decode, EbInputResolution resolution, bool rtc_tune) {
#else
static uint8_t get_dlf_level(EncMode enc_mode, uint8_t is_used_as_reference_flag, uint8_t is_16bit, Bool fast_decode, EbInputResolution resolution) {
#endif

    uint8_t dlf_level;
    // Don't disable DLF for low resolutions when fast-decode is used
    if (fast_decode == 0 || resolution <= INPUT_SIZE_360p_RANGE) {
        if (enc_mode <= ENC_M5)
        dlf_level = 1;
        else if (enc_mode <= ENC_M7)
            dlf_level = 2;
#if OPT_LD_M10
        else if ((enc_mode <= ENC_M8) || (rtc_tune && (enc_mode <= ENC_M10)))
#else
        else if (enc_mode <= ENC_M8)
#endif
            dlf_level = resolution <= INPUT_SIZE_360p_RANGE ? 2 : (is_used_as_reference_flag ? 2 : 4);
        else if (enc_mode <= ENC_M11)
            dlf_level = resolution <= INPUT_SIZE_360p_RANGE ? 2 : (is_used_as_reference_flag ? 3 : 0);
#if OPT_LD_M13
        else if (enc_mode <= ENC_M12 || rtc_tune)
#else
        else if (enc_mode <= ENC_M12)
#endif
            dlf_level = is_used_as_reference_flag ? 4 : 0;
        else
            dlf_level = (is_16bit && is_used_as_reference_flag) ? 4 : 0;
    }
    else {
        dlf_level = is_used_as_reference_flag ? 4 : 0;
    }

    return dlf_level;
}

uint8_t svt_aom_derive_gm_level(PictureParentControlSet* pcs) {
    SequenceControlSet* scs = pcs->scs;
    uint8_t gm_level = 0;
    const EncMode enc_mode = pcs->enc_mode;
    const uint8_t is_ref = pcs->is_used_as_reference_flag;

    // disable global motion when reference scaling enabled,
    // even if current pic is not scaled, because its reference
    // pics might be scaled in different size
    // super-res is ok for its reference pics are always upscaled
    // to original size
    if (scs->enable_global_motion == TRUE &&
        pcs->frame_superres_enabled == FALSE &&
        scs->static_config.resize_mode == RESIZE_NONE) {
        if (pcs->enc_mode <= ENC_MRS)
            gm_level = 2;
        else if (pcs->enc_mode <= ENC_M1)
            gm_level = 3;
        else if (enc_mode <= ENC_M3)
            gm_level = 4;
        else if (enc_mode <= ENC_M4)
            gm_level = 5;
        else if (pcs->enc_mode <= ENC_M6)
            gm_level = is_ref ? 6 : 0;
        else
            gm_level = 0;
    }
    return gm_level;
}

/******************************************************
* Derive Multi-Processes Settings for OQ
Input   : encoder mode and tune
Output  : Multi-Processes signal(s)
******************************************************/
EbErrorType signal_derivation_multi_processes_oq(
    SequenceControlSet *scs,
    PictureParentControlSet *pcs,
    PictureDecisionContext *context_ptr) {
    EbErrorType return_error = EB_ErrorNone;
    FrameHeader *frm_hdr = &pcs->frm_hdr;
    EncMode                enc_mode = pcs->enc_mode;
    const uint8_t            is_islice = pcs->slice_type == I_SLICE;
    const uint8_t            is_ref = pcs->is_used_as_reference_flag;
    const uint8_t            is_base = pcs->temporal_layer_index == 0;
    const EbInputResolution  input_resolution = pcs->input_resolution;
    const uint32_t           bit_depth = scs->static_config.encoder_bit_depth;
    const Bool               fast_decode = scs->static_config.fast_decode;
#if FIX_LAYER_SIGNAL
    const uint32_t           hierarchical_levels = pcs->hierarchical_levels;
#else
    const uint32_t           hierarchical_levels = scs->static_config.hierarchical_levels;
#endif
#if OPT_LD_M13
    const bool               rtc_tune = (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) ? true : false;
#endif
    // If enabled here, the hme enable flags should also be enabled in ResourceCoordinationProcess
    // to ensure that resources are allocated for the downsampled pictures used in HME
    pcs->enable_hme_flag        = 1;
    pcs->enable_hme_level0_flag = 1;
    if (pcs->sc_class1) {
        pcs->enable_hme_level1_flag = 1;
        pcs->enable_hme_level2_flag = 1;
    }
    else if (enc_mode <= ENC_M7) {
        pcs->enable_hme_level1_flag = 1;
        pcs->enable_hme_level2_flag = 1;
    }
    else {
        pcs->enable_hme_level1_flag = 1;
        pcs->enable_hme_level2_flag = 0;
    }
    switch (pcs->tf_ctrls.hme_me_level) {
    case 0:
        pcs->tf_enable_hme_flag        = 1;
        pcs->tf_enable_hme_level0_flag = 1;
        pcs->tf_enable_hme_level1_flag = 1;
        pcs->tf_enable_hme_level2_flag = 1;
        break;

    case 1:
        pcs->tf_enable_hme_flag        = 1;
        pcs->tf_enable_hme_level0_flag = 1;
        pcs->tf_enable_hme_level1_flag = 1;
        pcs->tf_enable_hme_level2_flag = 0;
        break;

    case 2:
#if OPT_LD_TF
    case 3:
#endif
        pcs->tf_enable_hme_flag       =  1;
        pcs->tf_enable_hme_level0_flag = 1;
        pcs->tf_enable_hme_level1_flag = 0;
        pcs->tf_enable_hme_level2_flag = 0;
        break;

    default:
        assert(0);
        break;
    }
    // Set the Multi-Pass PD level
    pcs->multi_pass_pd_level = MULTI_PASS_PD_ON;
    // Loop filter Level                            Settings
    // 0                                            OFF
    // 1                                            CU-BASED
    // 2                                            LIGHT FRAME-BASED
    // 3                                            FULL FRAME-BASED

    //for now only I frames are allowed to use sc tools.
    //TODO: we can force all frames in GOP with the same detection status of leading I frame.
    uint8_t intrabc_level = 0;
    if (is_islice) {
        frm_hdr->allow_screen_content_tools = pcs->sc_class1;
        if (scs->intrabc_mode == DEFAULT) {
            if (enc_mode <= ENC_M5)
                intrabc_level = 1;
            else if (enc_mode <= ENC_M7)
                intrabc_level = 4;
            else if (enc_mode <= ENC_M9)
                intrabc_level = 5;
            else if (enc_mode <= ENC_M10)
                intrabc_level = 6;
            else
                intrabc_level = 0;
        }
        else {
            intrabc_level = (uint8_t)scs->intrabc_mode;
        }
    }
    else {
        //this will enable sc tools for P frames. hence change Bitstream even if palette mode is OFF
        frm_hdr->allow_screen_content_tools = pcs->sc_class1;
        intrabc_level = 0;
    }
    set_intrabc_level(pcs, scs, intrabc_level);
    frm_hdr->allow_intrabc = pcs->intraBC_ctrls.enabled;
    // Set palette_level
    if (frm_hdr->allow_screen_content_tools) {
        if (scs->palette_level == DEFAULT) { //auto mode; if not set by cfg
            if (enc_mode <= ENC_M11)
                pcs->palette_level = is_base ? 2 : 0;
            else if (enc_mode <= ENC_M12)
                pcs->palette_level = is_islice ? 2 : 0;
            else
                pcs->palette_level = 0;
        }
        else
            pcs->palette_level = scs->palette_level;
    }
    else
        pcs->palette_level = 0;

    set_palette_level(pcs, pcs->palette_level);
    uint8_t dlf_level = 0;
    if (pcs->scs->static_config.enable_dlf_flag && frm_hdr->allow_intrabc == 0) {
#if OPT_LD_M9
        dlf_level = get_dlf_level(enc_mode, is_ref, bit_depth > EB_EIGHT_BIT, fast_decode, input_resolution, rtc_tune);
#else
        dlf_level = get_dlf_level(enc_mode, is_ref, bit_depth > EB_EIGHT_BIT, fast_decode, input_resolution);
#endif
    }
    svt_aom_set_dlf_controls(pcs, dlf_level, bit_depth);

    // Set CDEF controls
    if (scs->seq_header.cdef_level && frm_hdr->allow_intrabc == 0) {
        if (scs->static_config.cdef_level == DEFAULT) {
            if (enc_mode <= ENC_M0)
                pcs->cdef_level = 1;
            else if (enc_mode <= ENC_M4)
                pcs->cdef_level = 2;
            else if (enc_mode <= ENC_M5)
                pcs->cdef_level = 4;
            else if (enc_mode <= ENC_M7)
                pcs->cdef_level = is_ref ? 8 : 10;
            else if (enc_mode <= ENC_M10)
                pcs->cdef_level = is_base ? 8 : is_ref ? 9 : 10;
            else if (enc_mode <= ENC_M11)
                pcs->cdef_level = is_base ? 15 : is_ref ? 16 : 17;
            else if (enc_mode <= ENC_M13) {
                if (input_resolution <= INPUT_SIZE_1080p_RANGE)
                    pcs->cdef_level = is_base ? 15 : is_ref ? 16 : 17;
                else
                    pcs->cdef_level = is_islice ? 15 : is_ref ? 16 : 17;
            }
            else {
                if (hierarchical_levels <= 2) {
                    pcs->cdef_level = is_islice ? 15 : is_base ? 16 : 0;
                }
                else {
                if (input_resolution <= INPUT_SIZE_1080p_RANGE)
                    pcs->cdef_level = is_base ? 15 : 0;
                else
                    pcs->cdef_level = is_islice ? 15 : 0;
                }
            }
        }
        else
            pcs->cdef_level = (int8_t)(scs->static_config.cdef_level);
    }
    else
        pcs->cdef_level = 0;
    set_cdef_controls(pcs, pcs->cdef_level, fast_decode);

    uint8_t wn = 0, sg = 0;
    // If restoration filtering is enabled at the sequence level, derive the settings used for this frame
    if (scs->seq_header.enable_restoration) {
        wn = svt_aom_get_wn_filter_level(enc_mode, input_resolution, is_ref);
        sg = svt_aom_get_sg_filter_level(enc_mode, fast_decode, input_resolution, is_base);
    }

    Av1Common* cm = pcs->av1_cm;
    svt_aom_set_wn_filter_ctrls(cm, wn);
    svt_aom_set_sg_filter_ctrls(cm, sg);

    // Set whether restoration filtering is enabled for this frame
    pcs->enable_restoration = (wn > 0 || sg > 0);


    // Set frame end cdf update mode      Settings
    // 0                                     OFF
    // 1                                     ON
    if (scs->frame_end_cdf_update == DEFAULT)
        pcs->frame_end_cdf_update_mode = 1;
    else
        pcs->frame_end_cdf_update_mode = scs->frame_end_cdf_update;

    (void)context_ptr;

    // Tune TPL for better chroma.Only for 240P. 0 is OFF
#if TUNE_CHROMA_SSIM
    pcs->tune_tpl_for_chroma = 1;
#else
    pcs->tune_tpl_for_chroma = 0;
#endif
    uint8_t list0_only_base = 0;
    if (enc_mode <= ENC_M6)
        list0_only_base = 0;
    else if (enc_mode <= ENC_M7) {
        if (hierarchical_levels <= 3)
            list0_only_base = 1;
        else
            list0_only_base = 0;
    }
    else  if (enc_mode <= ENC_M9) {
        if (hierarchical_levels <= 3)
            list0_only_base = 2;
        else
            list0_only_base = 1;
    }
    else
        list0_only_base = 2;

    set_list0_only_base(pcs, list0_only_base);

    if (scs->enable_hbd_mode_decision == DEFAULT)
        if (enc_mode <= ENC_MR)
            pcs->hbd_md = 1;
        else if (enc_mode <= ENC_M1)
            pcs->hbd_md = is_ref ? 1 : 2;
        else if (enc_mode <= ENC_M3)
            pcs->hbd_md = 2;
        else if (enc_mode <= ENC_M4)
            pcs->hbd_md = is_ref ? 2 : 0;
        else if (enc_mode <= ENC_M7)
            pcs->hbd_md = is_base ? 2 : 0;
        else
            pcs->hbd_md = is_islice ? 2 : 0;
    else
        pcs->hbd_md = scs->enable_hbd_mode_decision;

    pcs->max_can_count = svt_aom_get_max_can_count(enc_mode);
    if (enc_mode <= ENC_M6)
        pcs->use_best_me_unipred_cand_only = 0;
    else
        pcs->use_best_me_unipred_cand_only = 1;

    //TPL level should not be modified outside of this function
    set_tpl_extended_controls(pcs, scs->tpl_level);
    pcs->r0_based_qps_qpm = pcs->tpl_ctrls.enable &&
                                (pcs->temporal_layer_index == 0 ||
                                (scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF && pcs->hierarchical_levels == 5 && pcs->temporal_layer_index == 1));
    pcs->adjust_under_shoot_gf = 0;
    if (scs->passes == 1 && scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_VBR)
        pcs->adjust_under_shoot_gf = enc_mode <= ENC_M11 ? 1 : 2;
    return return_error;
}

/*
* return the max canidate count for MDS0
  Used by candidate injection and memory allocation
*/
uint16_t svt_aom_get_max_can_count(EncMode enc_mode) {
    //NOTE: this is a memory feature and not a speed feature. it should not be have any speed/quality impact.
    uint16_t mem_max_can_count;

    if (enc_mode <= ENC_M0)
        mem_max_can_count = 1225;
    else if (enc_mode <= ENC_M1)
        mem_max_can_count = 1000;
    else if (enc_mode <= ENC_M2)
        mem_max_can_count = 720;
    else if (enc_mode <= ENC_M3)
        mem_max_can_count = 576;
    else if (enc_mode <= ENC_M4)
        mem_max_can_count = 369;
    else if (enc_mode <= ENC_M5)
        mem_max_can_count = 295;
    else if (enc_mode <= ENC_M6)
        mem_max_can_count = 236;
    else if (enc_mode <= ENC_M11)
        mem_max_can_count = 190;
    else
        mem_max_can_count = 80;

    return mem_max_can_count;
}


/*
 * Derive TF Params
 */
static void derive_tf_params(SequenceControlSet *scs) {

    // Do not perform TF if LD or 1 Layer or 1st pass
    Bool do_tf = scs->static_config.enable_tf && scs->static_config.hierarchical_levels >= 1 && scs->static_config.pass != ENC_FIRST_PASS;
    const EncMode enc_mode = scs->static_config.enc_mode;
    const uint32_t hierarchical_levels = scs->static_config.hierarchical_levels;
    uint8_t tf_level = 0;
#if OPT_LD_TF
    if (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_P || scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) {
        if (do_tf == 0)
            tf_level = 0;
        else
            tf_level = enc_mode <= ENC_M10 ? 1 : enc_mode <= ENC_M12 && (scs->input_resolution >= INPUT_SIZE_720p_RANGE) ? 2 : 0;

        tf_ld_controls(scs, tf_level);
        return;
    }
#endif
    if (do_tf == 0) {
        tf_level = 0;
    }
    else if (enc_mode <= ENC_MR) {
        tf_level = 1;
    }
    else if (enc_mode <= ENC_M5) {
        tf_level = 2;
    }
    else if (enc_mode <= ENC_M7) {
        tf_level = 3;
    }
    else if (enc_mode <= ENC_M8) {
        if (hierarchical_levels <= 3)
            tf_level = 4;
        else
            tf_level = 3;
    }
    else if (enc_mode <= ENC_M9) {
        tf_level = 4;
    }
    else
        tf_level = 5;

    tf_controls(scs, tf_level);
}

static uint8_t get_tpl_level(int8_t enc_mode, int32_t pass, int32_t lap_rc, uint8_t pred_structure, uint8_t superres_mode, uint8_t resize_mode, uint8_t aq_mode) {

    uint8_t tpl_level;

    if (aq_mode == 0) {
        SVT_WARN("TPL is disabled for aq_mode 0\n");
        tpl_level = 0;
    }
    else if (pred_structure == SVT_AV1_PRED_LOW_DELAY_B) {
        SVT_WARN("TPL is disabled in low delay applications.\n");
        tpl_level = 0;
    }
    else if (pass == ENC_FIRST_PASS && lap_rc == 0) {
        tpl_level = 0;
    }
    // allow TPL with auto-dual and auto-all
    else if (superres_mode > SUPERRES_NONE && superres_mode != SUPERRES_AUTO && superres_mode != SUPERRES_QTHRESH) {
        SVT_WARN("TPL will be disabled when super resolution is enabled!\n");
        tpl_level = 0;
    }
    else if (resize_mode > RESIZE_NONE) {
        SVT_WARN("TPL will be disabled when reference scalings (resize) is enabled!\n");
        tpl_level = 0;
    }
    else if (enc_mode <= ENC_M4)
        tpl_level = 1;
    else if (enc_mode <= ENC_M6)
        tpl_level = 2;
    else if (enc_mode <= ENC_M8)
        tpl_level = 4;
    else if (enc_mode <= ENC_M9)
        tpl_level = 5;
    else if (enc_mode <= ENC_M10)
        tpl_level = 6;
    else if (enc_mode <= ENC_M11)
        tpl_level = 7;
    else
        tpl_level = 8;

    return tpl_level;
}


/*
* Set multi Pass Params
*/
void set_multi_pass_params(SequenceControlSet *scs)
{
    EbSvtAv1EncConfiguration *config = &scs->static_config;

    // Update passes
    if (scs->static_config.pass != ENC_SINGLE_PASS)
        scs->passes = (scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_VBR) ? 3 : 2;
    else
        scs->passes = 1;

    if (config->rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF && config->pass == 2)
        scs->static_config.pass = 3; //use last pass for 2nd pass CRF

    switch (config->pass) {

        case ENC_SINGLE_PASS: {
            if (config->enc_mode <= ENC_M9)
                set_ipp_pass_ctrls(scs, 0);
            else
                set_ipp_pass_ctrls(scs, 1);
            set_mid_pass_ctrls(scs, 0);
            scs->ipp_was_ds = 0;
            scs->final_pass_preset = config->enc_mode;
            break;
        }

        case ENC_FIRST_PASS: {
            if (config->enc_mode <= ENC_M4)
                set_ipp_pass_ctrls(scs, 0);
            else if (config->enc_mode <= ENC_M10)
                set_ipp_pass_ctrls(scs, 2);
            else
                if (config->rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF)
                    set_ipp_pass_ctrls(scs, 3);
                else
                    set_ipp_pass_ctrls(scs, 2);
            set_mid_pass_ctrls(scs, 0);
            scs->ipp_was_ds = 0;
            scs->final_pass_preset = config->enc_mode;
            scs->static_config.enc_mode = MAX_ENC_PRESET;
            scs->static_config.look_ahead_distance = 0;
            scs->static_config.rate_control_mode = SVT_AV1_RC_MODE_CQP_OR_CRF;
            scs->static_config.intra_refresh_type = SVT_AV1_KF_REFRESH;
            scs->static_config.max_bit_rate = 0;
            scs->static_config.hierarchical_levels = 0;
            break;
        }

        case ENC_MIDDLE_PASS: {
            set_ipp_pass_ctrls(scs, 0);
            if (config->enc_mode <= ENC_M10)
                set_mid_pass_ctrls(scs, 0);
            else
                set_mid_pass_ctrls(scs, 1);
            scs->ipp_was_ds = 0;
            scs->final_pass_preset = config->enc_mode;
            if (scs->final_pass_preset <= ENC_M7)
                scs->static_config.enc_mode = ENC_M11;
            else if (scs->final_pass_preset <= ENC_M9)
                scs->static_config.enc_mode = ENC_M12;
            else
                scs->static_config.enc_mode = MAX_ENC_PRESET;
            scs->static_config.rate_control_mode = SVT_AV1_RC_MODE_CQP_OR_CRF;
            scs->static_config.qp = 43;
            scs->static_config.intra_refresh_type = SVT_AV1_KF_REFRESH;
            scs->static_config.max_bit_rate = 0;
            break;
        }

        case ENC_LAST_PASS: {
            set_ipp_pass_ctrls(scs, 0);
            // Please make sure that ipp_was_ds is ON only when ipp_ctrls->ipp_ds is ON
            if (config->enc_mode <= ENC_M10)
                scs->ipp_was_ds = 0;
            else
                scs->ipp_was_ds = config->rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF ? 1 : 0;
            scs->final_pass_preset = config->enc_mode;
            scs->static_config.intra_refresh_type = SVT_AV1_KF_REFRESH;
            break;
        }
        default: {
            assert(0);
            break;
        }
    }

    int do_downsample =
        (scs->mid_pass_ctrls.ds || scs->ipp_pass_ctrls.ds) && scs->max_input_luma_width >= 128 && scs->max_input_luma_height >= 128
        ? 1
        : 0;

    if (do_downsample) {
        // To make sure the down scaled video has width and height of multiple of 2
        scs->max_input_luma_width = (scs->max_input_luma_width >> 2) << 1;
        scs->max_input_luma_height = (scs->max_input_luma_height >> 2) << 1;
    }

    if (scs->lap_rc) {
        scs->static_config.intra_refresh_type = SVT_AV1_KF_REFRESH;
    }

    if (scs->static_config.pass == ENC_MIDDLE_PASS && scs->final_pass_preset > ENC_M8)
        scs->rc_stat_gen_pass_mode = 1;
    else
        scs->rc_stat_gen_pass_mode = 0;

    if (scs->static_config.recode_loop > 0 &&
        ((scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF && scs->static_config.max_bit_rate == 0) ||
        (scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_CBR))) {
        // Only allow re-encoding for VBR or capped CRF, otherwise force recode_loop to DISALLOW_RECODE or 0
        scs->static_config.recode_loop = DISALLOW_RECODE;
    }
    else if (scs->static_config.recode_loop == ALLOW_RECODE_DEFAULT) {
        // capped CRF has reencde enabled for base layer frames for all presets
        if (!scs->static_config.rate_control_mode && scs->static_config.max_bit_rate)
            scs->static_config.recode_loop = ALLOW_RECODE_KFARFGF;
        else
            scs->static_config.recode_loop = scs->static_config.enc_mode <= ENC_M2 ? ALLOW_RECODE_KFARFGF : ALLOW_RECODE_KFMAXBW;
    }

    scs->encode_context_ptr->recode_loop = scs->static_config.recode_loop;
}


static void set_param_based_on_input(SequenceControlSet *scs)
{
    set_multi_pass_params(
        scs);

    // superres_mode and resize_mode may be updated,
    // so should call get_tpl_level() after validate_scaling_params()
    validate_scaling_params(scs);

    scs->tpl_level = get_tpl_level(scs->static_config.enc_mode, scs->static_config.pass, scs->lap_rc, scs->static_config.pred_structure, scs->static_config.superres_mode, scs->static_config.resize_mode, scs->static_config.enable_adaptive_quantization);

    uint16_t subsampling_x = scs->subsampling_x;
    uint16_t subsampling_y = scs->subsampling_y;
    // Update picture width, and picture height
    if (scs->max_input_luma_width % MIN_BLOCK_SIZE) {
        scs->max_input_pad_right = MIN_BLOCK_SIZE - (scs->max_input_luma_width % MIN_BLOCK_SIZE);
        scs->max_input_luma_width = scs->max_input_luma_width + scs->max_input_pad_right;
    } else {
        scs->max_input_pad_right = 0;
    }

    if (scs->max_input_luma_height % MIN_BLOCK_SIZE) {
        scs->max_input_pad_bottom = MIN_BLOCK_SIZE - (scs->max_input_luma_height % MIN_BLOCK_SIZE);
        scs->max_input_luma_height = scs->max_input_luma_height + scs->max_input_pad_bottom;
    } else {
        scs->max_input_pad_bottom = 0;
    }

    scs->max_input_chroma_width = scs->max_input_luma_width >> subsampling_x;
    scs->max_input_chroma_height = scs->max_input_luma_height >> subsampling_y;

    scs->chroma_width = scs->max_input_luma_width >> subsampling_x;
    scs->chroma_height = scs->max_input_luma_height >> subsampling_y;
    scs->static_config.source_width = scs->max_input_luma_width;
    scs->static_config.source_height = scs->max_input_luma_height;
    if (scs->static_config.superres_mode > SUPERRES_NONE) {
        if (scs->static_config.tile_rows || scs->static_config.tile_columns) {
            // disable tiles if super-res is on
            SVT_WARN("Tiles will be disabled when super resolution is enabled!\n");
            scs->static_config.tile_rows = 0;
            scs->static_config.tile_columns = 0;
        }
        if (scs->static_config.superres_mode == SUPERRES_RANDOM) {
            SVT_WARN("Super resolution random mode is designed for test and debugging purpose,\n"
                "it creates array of picture buffers for all scaling denominators (up to 8) of each reference frame.\n"
                "This mode retains a significant amount of memory, much more than other modes!\n");
        }
    }
    if (scs->static_config.resize_mode > RESIZE_NONE) {
        if (scs->static_config.tile_rows || scs->static_config.tile_columns) {
            // disable tiles if resize is on
            SVT_WARN("Tiles will be disabled when resize is enabled!\n");
            scs->static_config.tile_rows = 0;
            scs->static_config.tile_columns = 0;
        }
        if (scs->static_config.resize_mode == RESIZE_RANDOM) {
            SVT_WARN("Resize random mode is designed for test and debugging purpose,\n"
                "it creates array of picture buffers for all scaling denominators (up to 8) of each reference frame.\n"
                "This mode retains a significant amount of memory, much more than other modes!\n");
        }
    }
    // Set initial qp for single pass vbr
    if ((scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_VBR) && (scs->static_config.pass == ENC_SINGLE_PASS)){
        if (scs->static_config.qp != DEFAULT_QP) {
            SVT_WARN("The input q value is ignored in vbr mode %d\n", scs->static_config.qp);
        }
        const uint8_t tbr_bands[6] = { 1,2,4,6,8,10 };
        const uint64_t src_samples = (uint64_t)(scs->seq_header.max_frame_width*scs->seq_header.max_frame_height);
        const uint64_t target_bit_rate = scs->static_config.target_bit_rate / src_samples;
        if (target_bit_rate < tbr_bands[0])
            scs->static_config.qp = 50;
        else if (target_bit_rate < tbr_bands[1])
            scs->static_config.qp = 45;
        else if (target_bit_rate < tbr_bands[2])
            scs->static_config.qp = 40;
        else if (target_bit_rate < tbr_bands[3])
            scs->static_config.qp = 35;
        else if (target_bit_rate < tbr_bands[4])
            scs->static_config.qp = 30;
        else if (target_bit_rate < tbr_bands[5])
            scs->static_config.qp = 25;
        else
            scs->static_config.qp = 20;
    }
    svt_aom_derive_input_resolution(
        &scs->input_resolution,
        scs->max_input_luma_width *scs->max_input_luma_height);
    // Set tune params
    derive_vq_params(scs);

    // Set TF level
    derive_tf_params(scs);

    //Future frames window in Scene Change Detection (SCD) / TemporalFiltering
    scs->scd_delay = 0;

    // Update the scd_delay based on the the number of future frames @ ISLICE
    // This case is needed for non-delayed Intra (intra_period_length == 0)
    uint32_t scd_delay_islice = 0;
    if (scs->static_config.intra_period_length == 0)
        if (scs->tf_params_per_type[0].enabled)
            scd_delay_islice =
            MIN(scs->tf_params_per_type[0].num_future_pics + (scs->tf_params_per_type[0].noise_adjust_future_pics ? 3 : 0), // number of future picture(s) used for ISLICE + max picture(s) after noise-based adjustement (=3)
                scs->tf_params_per_type[0].max_num_future_pics);


    // Update the scd_delay based on the the number of future frames @ BASE
    uint32_t scd_delay_base = 0;
    if (scs->tf_params_per_type[1].enabled)
        scd_delay_base =
        MIN(scs->tf_params_per_type[1].num_future_pics + (scs->tf_params_per_type[1].noise_adjust_future_pics ? 3 : 0), // number of future picture(s) used for BASE + max picture(s) after noise-based adjustement (=3)
            scs->tf_params_per_type[1].max_num_future_pics);
    scs->scd_delay = MAX(scd_delay_islice, scd_delay_base);

    // Update the scd_delay based on SCD, 1first pass
    // Delay needed for SCD , 1first pass of (2pass and 1pass VBR)
    if (scs->static_config.scene_change_detection || scs->vq_ctrls.sharpness_ctrls.scene_transition || scs->static_config.pass == ENC_FIRST_PASS || scs->lap_rc)
        scs->scd_delay = MAX(scs->scd_delay, 2);

    // no future minigop is used for lowdelay prediction structure
    if (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_P || scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) {
        scs->lad_mg = scs->tpl_lad_mg = 0;
    }
    else
     {
        uint8_t tpl_lad_mg = 1; // Specify the number of mini-gops to be used as LAD. 0: 1 mini-gop, 1: 2 mini-gops and 3: 3 mini-gops
        uint32_t mg_size = 1 << scs->static_config.hierarchical_levels;
        // If the lookahead is specified to be less than one mini-gop, then use only the current mini-gop for TPL (the current MG is always required to encode).
        // Otherwise, set tpl_lad_mg to 1 when TPL is used, regardless of teh specified lookahead, because TPL has been optimized to use 1 MG lookahead. Using
        // more lookahead MGs may result in disadvantageous trade-offs (speed/BDR/memory).
        if (scs->static_config.look_ahead_distance < mg_size)
            tpl_lad_mg = 0;
        else if (scs->tpl_level)
            tpl_lad_mg = 1;
        else
            tpl_lad_mg = 0;

        // special conditions for higher resolutions in order to decrease memory usage for tpl_lad_mg
#if !REMOVE_LP1_LPN_DIFF
        if (scs->static_config.logical_processors == 1 && scs->input_resolution >= INPUT_SIZE_4K_RANGE && scs->static_config.hierarchical_levels >= 4) {
            tpl_lad_mg = 0;
        }
        else
#endif
        if (scs->input_resolution >= INPUT_SIZE_8K_RANGE && scs->static_config.hierarchical_levels >= 4) {
            tpl_lad_mg = 0;
        }
        scs->tpl_lad_mg = MIN(2, tpl_lad_mg);// lad_mg is capped to 2 because tpl was optimised only for 1,2 and 3 mini-gops
        if (scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF)
            scs->lad_mg = scs->tpl_lad_mg;
        else
            // update the look ahead size
            update_look_ahead(scs);
    }
#if OPT_LD
    // In low delay mode, sb size is set to 64
    // In two pass encoding, the first pass uses sb size=64. Also when tpl is used
    // in 240P resolution, sb size is set to 64
    if (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B ||
        scs->static_config.pass == ENC_FIRST_PASS ||
        (scs->tpl_level && scs->input_resolution == INPUT_SIZE_240p_RANGE))
#else
    // In two pass encoding, the first pass uses sb size=64. Also when tpl is used
    // in 240P resolution, sb size is set to 64
    if (scs->static_config.pass == ENC_FIRST_PASS ||
        (scs->tpl_level && scs->input_resolution == INPUT_SIZE_240p_RANGE))
#endif
        scs->super_block_size = 64;
    else
        if (scs->static_config.enc_mode <= ENC_M2)
            scs->super_block_size = 128;
        else
            scs->super_block_size = 64;
    // When switch frame is on, all renditions must have same super block size. See spec 5.5.1, 5.9.15.
    if (scs->static_config.pass != ENC_FIRST_PASS && scs->static_config.sframe_dist != 0)
        scs->super_block_size = 64;
    // Set config info related to SB size
    if (scs->super_block_size == 128) {
        scs->seq_header.sb_size = BLOCK_128X128;
        scs->sb_size = 128;
        scs->seq_header.sb_mi_size = 32; // Size of the superblock in units of MI blocks
        scs->seq_header.sb_size_log2 = 5;
    }
    else {
        scs->seq_header.sb_size = BLOCK_64X64;
        scs->sb_size = 64;
        scs->seq_header.sb_mi_size = 16; // Size of the superblock in units of MI blocks
        scs->seq_header.sb_size_log2 = 4;
    }
    // scs->static_config.hierarchical_levels = (scs->static_config.rate_control_mode > 1) ? 3 : scs->static_config.hierarchical_levels;
    if (scs->static_config.restricted_motion_vector && scs->super_block_size == 128) {
        scs->static_config.restricted_motion_vector = FALSE;
        SVT_WARN("Restricted_motion_vector and SB 128x128 not supoorted, setting rmv to false\n");
    }
    if (scs->static_config.intra_refresh_type == SVT_AV1_FWDKF_REFRESH && scs->static_config.hierarchical_levels != 4){
        scs->static_config.hierarchical_levels = 4;
        SVT_WARN("Fwd key frame is only supported for hierarchical levels 4 at this point. Hierarchical levels are set to 4\n");
    }
    bool disallow_nsq = true;
    for (uint8_t is_base = 0; is_base <= 1; is_base++)
        for (uint8_t is_islice = 0; is_islice <= 1; is_islice++)
            for (uint8_t coeff_lvl = 0; coeff_lvl <= HIGH_LVL + 1; coeff_lvl++)
                disallow_nsq = MIN(disallow_nsq, (svt_aom_get_nsq_level(scs->static_config.enc_mode, is_islice, is_base, coeff_lvl) == 0 ? 1 : 0));
    bool disallow_4x4 = true;
    for (SliceType slice_type = 0; slice_type < IDR_SLICE + 1; slice_type++)
        disallow_4x4 = MIN(disallow_4x4, svt_aom_get_disallow_4x4(scs->static_config.enc_mode, slice_type));
    if (scs->super_block_size == 128) {
        scs->svt_aom_geom_idx = GEOM_2;
        scs->max_block_cnt = 4421;
    }
    else {
        //SB 64x64
        if (disallow_nsq && disallow_4x4) {
            scs->svt_aom_geom_idx = GEOM_0;
            scs->max_block_cnt = 85;
        }
        else {
            scs->svt_aom_geom_idx = GEOM_1;
            scs->max_block_cnt = 1101;
        }
    }
    //printf("\n\nGEOM:%i \n", scs->svt_aom_geom_idx);
    // Configure the padding
    scs->left_padding = BLOCK_SIZE_64 + 4;
    scs->top_padding = BLOCK_SIZE_64 + 4;
    scs->right_padding = BLOCK_SIZE_64 + 4;
    scs->bot_padding = scs->super_block_size + 4;

    //for 10bit,  increase the pad of source from 68 to 72 (mutliple of 8) to accomodate 2bit-compression flow
    //we actually need to change the horizontal dimension only, but for simplicity/uniformity we do all directions
   // if (scs->static_config.encoder_bit_depth != EB_EIGHT_BIT)
    {
        scs->left_padding  += 4;
        scs->top_padding   += 4;
        scs->right_padding += 4;
        scs->bot_padding   += 4;
    }


    scs->static_config.enable_overlays = !scs->static_config.enable_tf ||
        (scs->static_config.rate_control_mode != SVT_AV1_RC_MODE_CQP_OR_CRF) ?
        0 : scs->static_config.enable_overlays;
    //0: ON
    //1: OFF
    // Memory Footprint reduction tool ONLY if no CDF (should be controlled using an API signal and not f(enc_mode))
    scs->cdf_mode = 0;
    // Set down-sampling method     Settings
    // 0                            0: filtering
    // 1                            1: decimation

    scs->down_sampling_method_me_search = ME_FILTERED_DOWNSAMPLED;

    // Enforce starting frame in decode order (at PicMgr)
    // Does not wait for feedback from PKT
    if (scs->static_config.logical_processors == 1)
        scs->enable_pic_mgr_dec_order = 1;
    else
        scs->enable_pic_mgr_dec_order = 0;
    // Enforce encoding frame in decode order
    // Wait for feedback from PKT
#if RC_NO_R2R
    scs->enable_dec_order = 1;
#else
    if (scs->static_config.logical_processors == 1)
        scs->enable_dec_order = 1;
    else
        scs->enable_dec_order = 0;
#endif
   // Open loop intra done with TPL, data is not stored
    scs->in_loop_ois = 1;

    // 1: Use boundary pixels in restoration filter search.
    // 0: Do not use boundary pixels in the restoration filter search.
    scs->use_boundaries_in_rest_search = 0;

    // Set over_boundary_block_mode     Settings
    // 0                            0: not allowed
    // 1                            1: allowed
    if (scs->over_bndry_blk == DEFAULT)
        scs->over_boundary_block_mode = 1;
    else
        scs->over_boundary_block_mode = scs->over_bndry_blk;
    if (scs->static_config.pass == ENC_FIRST_PASS)
        scs->over_boundary_block_mode = 0;

    if (scs->static_config.enable_mfmv == DEFAULT) {
        if (scs->static_config.enc_mode <= ENC_M5)
            scs->mfmv_enabled = 1;
#if OPT_LD_M11
        else if (scs->static_config.enc_mode <= ENC_M10 ||
                (scs->static_config.pred_structure != SVT_AV1_PRED_LOW_DELAY_B &&
                scs->static_config.enc_mode <= ENC_M11)) {
#else
        else if (scs->static_config.enc_mode <= ENC_M11) {
#endif
            if (scs->input_resolution <= INPUT_SIZE_1080p_RANGE)
                scs->mfmv_enabled = 1;
            else
                scs->mfmv_enabled = 0;
        } else
            scs->mfmv_enabled = 0;
    } else
        scs->mfmv_enabled = scs->static_config.enable_mfmv;

    if (scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_VBR || scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_CBR ||
        scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B || scs->static_config.pass != ENC_SINGLE_PASS || scs->static_config.enc_mode >= ENC_M10)
        scs->enable_dg = 0;
    else
        scs->enable_dg = scs->static_config.enable_dg;


    // Set hbd_md OFF for high encode modes or bitdepth < 10
    if (scs->static_config.encoder_bit_depth < 10)
        scs->enable_hbd_mode_decision = 0;

    // Throws a warning when scene change is on, as the feature is not optimal and may produce false detections
    if (scs->static_config.scene_change_detection == 1)
        SVT_WARN("Scene Change is not optimal and may produce suboptimal keyframe placements\n");

    // MRP level
    uint8_t mrp_level;

    if (scs->static_config.enc_mode <= ENC_MRS) {
        mrp_level = 1;
    }
    else if (scs->static_config.enc_mode <= ENC_M1) {
        mrp_level = 2;
    }
    else if (scs->static_config.enc_mode <= ENC_M4) {
        mrp_level = 3;
    }
    else if (scs->static_config.enc_mode <= ENC_M5) {
        mrp_level = 4;
    }
    else if (scs->static_config.enc_mode <= ENC_M6) {
        mrp_level = 5;
    }
#if OPT_LD_MRP2
    else if (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B && scs->static_config.enc_mode <= ENC_M12) {
        mrp_level = 6;
    }
    else if (scs->static_config.enc_mode <= ENC_M7 || scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) {
        if (scs->static_config.hierarchical_levels <= 3)
            mrp_level = 7;
        else
            mrp_level = 5;
    }
    else if (scs->static_config.enc_mode <= ENC_M12) {
        mrp_level = 8;
    }
#else
#if OPT_LD_MRP
    else if (scs->static_config.enc_mode <= ENC_M7 || scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) {
#else
    else if (scs->static_config.enc_mode <= ENC_M7) {
#endif
        if (scs->static_config.hierarchical_levels <= 3)
            mrp_level = 6;
        else
            mrp_level = 5;
    }
    else if (scs->static_config.enc_mode <= ENC_M12) {
        mrp_level = 7;
    }
#endif
    else {
        mrp_level = 0;
    }

    set_mrp_ctrl(scs, mrp_level);
    scs->is_short_clip = scs->static_config.gop_constraint_rc ? 1 : 0; // set to 1 if multipass and less than 200 frames in resourcecordination

    // Variance is required for scene change detection and segmentation-based quantization and subjective mode tf control
    if (scs->static_config.enable_adaptive_quantization == 1 ||
        scs->static_config.scene_change_detection == 1       ||
        scs->vq_ctrls.sharpness_ctrls.tf == 1)
        scs->calculate_variance = 1;
    else if (scs->static_config.enc_mode <= ENC_M10)
        scs->calculate_variance = 1;
    else
        scs->calculate_variance = 0;

    scs->resize_pending_params.resize_state = ORIG;
    scs->resize_pending_params.resize_denom = SCALE_NUMERATOR;
    scs->stats_based_sb_lambda_modulation = scs->static_config.pred_structure == SVT_AV1_PRED_RANDOM_ACCESS &&
                                                scs->static_config.rate_control_mode != SVT_AV1_RC_MODE_CBR
        ? 1
        : 0;
#if OPT_LD_LATENCY_MD
    scs->low_latency_kf = ((scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_P
         || scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) &&
        scs->static_config.enc_mode <= ENC_M10)
        ? 1
        : 0;
#endif
}

static void copy_api_from_app(
    SequenceControlSet       *scs,
    EbSvtAv1EncConfiguration   *config_struct){

    scs->max_input_luma_width = config_struct->source_width;
    scs->max_input_luma_height = config_struct->source_height;
    // SB Definitions
    scs->static_config.pred_structure = ((EbSvtAv1EncConfiguration*)config_struct)->pred_structure;
    // Tpl is disabled in low delay applications
    if (scs->static_config.pred_structure == 0) {
        ((EbSvtAv1EncConfiguration*)config_struct)->enable_tpl_la = 0;
    }
    scs->enable_qp_scaling_flag = 1;
#if !CLN_REMOVE_REF_CNT
    scs->max_ref_count = 1;
    scs->reference_count = 4;
#endif

    // Set Picture Parameters for statistics gathering
    scs->picture_analysis_number_of_regions_per_width =
        HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_WIDTH;
    scs->picture_analysis_number_of_regions_per_height =
        HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_HEIGHT;

    scs->enable_warped_motion        = DEFAULT;
    scs->enable_global_motion        = TRUE;
    scs->inter_intra_compound        = DEFAULT;
    scs->enable_paeth                = DEFAULT;
    scs->enable_smooth               = DEFAULT;
    scs->spatial_sse_full_loop_level = DEFAULT;
    scs->over_bndry_blk              = DEFAULT;
    scs->new_nearest_comb_inject     = DEFAULT;
    scs->frame_end_cdf_update        = DEFAULT;
    scs->disable_cfl_flag            = DEFAULT;
    scs->obmc_level                  = DEFAULT;
    scs->rdoq_level                  = DEFAULT;
    scs->pred_me                     = DEFAULT;
    scs->bipred_3x3_inject           = DEFAULT;
    scs->compound_level              = DEFAULT;
    scs->filter_intra_level          = DEFAULT;
    scs->enable_intra_edge_filter    = DEFAULT;
    scs->pic_based_rate_est          = DEFAULT;
    scs->block_mean_calc_prec        = BLOCK_MEAN_PREC_SUB;
    scs->palette_level = DEFAULT;
    scs->intra_angle_delta = DEFAULT;
    scs->intrabc_mode = DEFAULT;
    scs->ten_bit_format = 0;
    scs->speed_control_flag = 0;

    // Padding Offsets
    scs->b64_size = 64;
    scs->static_config.intra_period_length = ((EbSvtAv1EncConfiguration*)config_struct)->intra_period_length;
    scs->static_config.multiply_keyint = config_struct->multiply_keyint;
    scs->static_config.intra_refresh_type = ((EbSvtAv1EncConfiguration*)config_struct)->intra_refresh_type;
    scs->static_config.enc_mode = ((EbSvtAv1EncConfiguration*)config_struct)->enc_mode;
    EbInputResolution input_resolution;
    svt_aom_derive_input_resolution(
        &input_resolution,
        scs->max_input_luma_width * scs->max_input_luma_height);
#if OPT_LD_M13
    if (scs->static_config.pred_structure == SVT_AV1_PRED_RANDOM_ACCESS && scs->static_config.enc_mode > ENC_M12 && input_resolution <= INPUT_SIZE_360p_RANGE) {
        scs->static_config.enc_mode = ENC_M12;
        SVT_WARN("Setting preset to M12 as it is the highest supported preset for 360p and lower resolutions in Random Access mode\n");
    }
#else
    if (scs->static_config.enc_mode > ENC_M12 && input_resolution <= INPUT_SIZE_360p_RANGE) {
        scs->static_config.enc_mode = ENC_M12;
        SVT_WARN("Setting preset to M12 as it is the highest supported preset for 360p and lower resolutions\n");
    }
#endif
    scs->static_config.use_qp_file = ((EbSvtAv1EncConfiguration*)config_struct)->use_qp_file;
    scs->static_config.use_fixed_qindex_offsets = ((EbSvtAv1EncConfiguration*)config_struct)->use_fixed_qindex_offsets;
    scs->static_config.key_frame_chroma_qindex_offset = ((EbSvtAv1EncConfiguration*)config_struct)->key_frame_chroma_qindex_offset;
    scs->static_config.key_frame_qindex_offset = ((EbSvtAv1EncConfiguration*)config_struct)->key_frame_qindex_offset;
    if (scs->static_config.use_fixed_qindex_offsets) {
        scs->enable_qp_scaling_flag = scs->static_config.use_fixed_qindex_offsets == 1
            ? 0
            : 1; // do not shut the auto QPS if use_fixed_qindex_offsets 2
        scs->static_config.use_qp_file = 0;
        memcpy(scs->static_config.qindex_offsets, ((EbSvtAv1EncConfiguration*)config_struct)->qindex_offsets,
            MAX_TEMPORAL_LAYERS * sizeof(int32_t));
    }
    memcpy(scs->static_config.chroma_qindex_offsets, config_struct->chroma_qindex_offsets,
        MAX_TEMPORAL_LAYERS * sizeof(int32_t));
    scs->static_config.luma_y_dc_qindex_offset =
      ((EbSvtAv1EncConfiguration*)config_struct)->luma_y_dc_qindex_offset;
    scs->static_config.chroma_u_dc_qindex_offset =
      ((EbSvtAv1EncConfiguration*)config_struct)->chroma_u_dc_qindex_offset;
    scs->static_config.chroma_u_ac_qindex_offset =
      ((EbSvtAv1EncConfiguration*)config_struct)->chroma_u_ac_qindex_offset;
    scs->static_config.chroma_v_dc_qindex_offset =
      ((EbSvtAv1EncConfiguration*)config_struct)->chroma_v_dc_qindex_offset;
    scs->static_config.chroma_v_ac_qindex_offset =
      ((EbSvtAv1EncConfiguration*)config_struct)->chroma_v_ac_qindex_offset;

    memcpy(scs->static_config.lambda_scale_factors, config_struct->lambda_scale_factors,
        SVT_AV1_FRAME_UPDATE_TYPES * sizeof(int32_t));

    scs->static_config.rc_stats_buffer = ((EbSvtAv1EncConfiguration*)config_struct)->rc_stats_buffer;
    scs->static_config.pass = ((EbSvtAv1EncConfiguration*)config_struct)->pass;
    // Deblock Filter
    scs->static_config.enable_dlf_flag = ((EbSvtAv1EncConfiguration*)config_struct)->enable_dlf_flag;

    // CDEF
    scs->static_config.cdef_level = ((EbSvtAv1EncConfiguration*)config_struct)->cdef_level;

    // Restoration filtering
    scs->static_config.enable_restoration_filtering = ((EbSvtAv1EncConfiguration*)config_struct)->enable_restoration_filtering;

    // motion field motion vector
    scs->static_config.enable_mfmv                  = ((EbSvtAv1EncConfiguration*)config_struct)->enable_mfmv;

    // Dynamic GoP
    scs->static_config.enable_dg = ((EbSvtAv1EncConfiguration*)config_struct)->enable_dg;

    // Decoder Optimization Flag
    scs->static_config.fast_decode = ((EbSvtAv1EncConfiguration*)config_struct)->fast_decode;

    // If the set fast_decode value is in the allowable range, check that the value is supported for the current preset.
    // If the value is valid, but not supported in the current preset, change the value to one that is supported.
    if (scs->static_config.fast_decode == 1) {
        if (scs->static_config.enc_mode <= ENC_MR || scs->static_config.enc_mode >= ENC_M11) {
            SVT_WARN("The fast decode option is not supported in M%d.\n", scs->static_config.enc_mode);
            SVT_WARN("Decoder speedup is only supported in presets M0-M10.\n");
            SVT_WARN("Switching off decoder speedup optimizations.\n");
            scs->static_config.fast_decode = 0;
        }
    }

    //Film Grain
    scs->static_config.film_grain_denoise_strength = ((EbSvtAv1EncConfiguration*)config_struct)->film_grain_denoise_strength;
    scs->static_config.film_grain_denoise_apply = ((EbSvtAv1EncConfiguration*)config_struct)->film_grain_denoise_apply;
    if (scs->static_config.film_grain_denoise_strength == 0 && scs->static_config.film_grain_denoise_apply == 0) {
        SVT_WARN("Film grain denoise apply signal is going to be ignored when film grain is off.\n");
    }
    scs->seq_header.film_grain_params_present = (uint8_t)(scs->static_config.film_grain_denoise_strength>0);

    // MD Parameters
    scs->enable_hbd_mode_decision = ((EbSvtAv1EncConfiguration*)config_struct)->encoder_bit_depth > 8 ? DEFAULT : 0;

    if (scs->static_config.pass == ENC_FIRST_PASS) {
        scs->static_config.tile_rows = 0;
        scs->static_config.tile_columns = 0;
    }
    else {
        if (((EbSvtAv1EncConfiguration*)config_struct)->tile_rows == DEFAULT && ((EbSvtAv1EncConfiguration*)config_struct)->tile_columns == DEFAULT) {

            scs->static_config.tile_rows = 0;
            scs->static_config.tile_columns = 0;

        }
        else {
            if (((EbSvtAv1EncConfiguration*)config_struct)->tile_rows == DEFAULT) {
                scs->static_config.tile_rows = 0;
                scs->static_config.tile_columns = ((EbSvtAv1EncConfiguration*)config_struct)->tile_columns;
            }
            else if (((EbSvtAv1EncConfiguration*)config_struct)->tile_columns == DEFAULT) {
                scs->static_config.tile_rows = ((EbSvtAv1EncConfiguration*)config_struct)->tile_rows;
                scs->static_config.tile_columns = 0;
            }
            else {
                scs->static_config.tile_rows = ((EbSvtAv1EncConfiguration*)config_struct)->tile_rows;
                scs->static_config.tile_columns = ((EbSvtAv1EncConfiguration*)config_struct)->tile_columns;
            }
        }
    }

    scs->static_config.restricted_motion_vector = ((EbSvtAv1EncConfiguration*)config_struct)->restricted_motion_vector;

    // Rate Control
    scs->static_config.scene_change_detection = ((EbSvtAv1EncConfiguration*)config_struct)->scene_change_detection;
    scs->static_config.rate_control_mode = ((EbSvtAv1EncConfiguration*)config_struct)->rate_control_mode;
    if (scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_CBR && scs->static_config.pred_structure == SVT_AV1_PRED_RANDOM_ACCESS) {
        SVT_WARN("CBR Rate control is currently not supported for SVT_AV1_PRED_RANDOM_ACCESS, switching to VBR\n");
        scs->static_config.rate_control_mode = SVT_AV1_RC_MODE_VBR;
    }
    if (scs->static_config.pass == ENC_SINGLE_PASS && scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) {
        if (scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_VBR) {
            scs->static_config.rate_control_mode = SVT_AV1_RC_MODE_CBR;
            SVT_WARN("Low delay mode does not support VBR. Forcing RC mode to CBR\n");
        }

        if (scs->static_config.enc_mode < ENC_M8) {
            scs->static_config.enc_mode = ENC_M8;
            SVT_WARN("Low delay mode only support encodermode [8-%d]. Forcing encoder mode to 8\n", ENC_M13);
        }
    }
    scs->static_config.tune = config_struct->tune;
    scs->static_config.hierarchical_levels = ((EbSvtAv1EncConfiguration*)config_struct)->hierarchical_levels;

    // Set the default hierarchical levels
    if (scs->static_config.hierarchical_levels == 0) {
        scs->static_config.hierarchical_levels = scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B ?
#if OPT_LD
            2 :
#else
            3 :
#endif
            scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_VBR || scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_CBR || !(scs->static_config.enc_mode <= ENC_M12)
                ? 4
                : 5;
    }
    if (scs->static_config.pass == ENC_SINGLE_PASS && scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) {
#if OPT_LD
        if (scs->static_config.hierarchical_levels != 2) {
            scs->static_config.hierarchical_levels = 2;
            SVT_WARN("Forced Low delay mode to use HierarchicalLevels = 2\n");
        }
#else
        if (scs->static_config.hierarchical_levels != 3) {
            scs->static_config.hierarchical_levels = 3;
            SVT_WARN("Forced Low delay mode to use HierarchicalLevels = 3\n");
        }
#endif
    }
    scs->max_temporal_layers = scs->static_config.hierarchical_levels;
    scs->static_config.look_ahead_distance = ((EbSvtAv1EncConfiguration*)config_struct)->look_ahead_distance;
    scs->static_config.frame_rate_denominator = ((EbSvtAv1EncConfiguration*)config_struct)->frame_rate_denominator;
    scs->static_config.frame_rate_numerator = ((EbSvtAv1EncConfiguration*)config_struct)->frame_rate_numerator;

    scs->static_config.target_bit_rate = ((EbSvtAv1EncConfiguration*)config_struct)->target_bit_rate;
    scs->static_config.max_bit_rate = ((EbSvtAv1EncConfiguration*)config_struct)->max_bit_rate;
    if (((EbSvtAv1EncConfiguration*)config_struct)->enable_tpl_la == 0 && scs->static_config.max_bit_rate) {
        scs->static_config.max_bit_rate = 0;
        SVT_WARN("Maximum bit rate only supported with tpl on. max bit rate 0 is used instead.\n");
    }
    scs->static_config.max_qp_allowed = (scs->static_config.rate_control_mode) ?
        ((EbSvtAv1EncConfiguration*)config_struct)->max_qp_allowed :
        63;

    scs->static_config.min_qp_allowed = (scs->static_config.rate_control_mode) ?
        (((EbSvtAv1EncConfiguration*)config_struct)->min_qp_allowed > 0) ?
        ((EbSvtAv1EncConfiguration*)config_struct)->min_qp_allowed : 1 :
        1; // lossless coding not supported

    scs->static_config.vbr_bias_pct        = ((EbSvtAv1EncConfiguration*)config_struct)->vbr_bias_pct;
    scs->static_config.vbr_min_section_pct = ((EbSvtAv1EncConfiguration*)config_struct)->vbr_min_section_pct;
    scs->static_config.vbr_max_section_pct = ((EbSvtAv1EncConfiguration*)config_struct)->vbr_max_section_pct;
    scs->static_config.under_shoot_pct     = ((EbSvtAv1EncConfiguration*)config_struct)->under_shoot_pct;
    scs->static_config.over_shoot_pct      = ((EbSvtAv1EncConfiguration*)config_struct)->over_shoot_pct;
    scs->static_config.mbr_over_shoot_pct  = ((EbSvtAv1EncConfiguration*)config_struct)->mbr_over_shoot_pct;
    scs->static_config.gop_constraint_rc   = ((EbSvtAv1EncConfiguration*)config_struct)->gop_constraint_rc;
    scs->static_config.maximum_buffer_size_ms   = ((EbSvtAv1EncConfiguration*)config_struct)->maximum_buffer_size_ms;
    scs->static_config.starting_buffer_level_ms = ((EbSvtAv1EncConfiguration*)config_struct)->starting_buffer_level_ms;
    scs->static_config.optimal_buffer_level_ms  = ((EbSvtAv1EncConfiguration*)config_struct)->optimal_buffer_level_ms;
    scs->static_config.recode_loop         = ((EbSvtAv1EncConfiguration*)config_struct)->recode_loop;
    if (scs->static_config.rate_control_mode == SVT_AV1_RC_MODE_VBR && scs->static_config.pass == ENC_SINGLE_PASS)
        scs->lap_rc = 1;
    else
        scs->lap_rc = 0;
    //Segmentation
    //TODO: check RC mode and set only when RC is enabled in the final version.
    scs->static_config.enable_adaptive_quantization = config_struct->enable_adaptive_quantization;

    // Misc
    scs->static_config.encoder_bit_depth = ((EbSvtAv1EncConfiguration*)config_struct)->encoder_bit_depth;
    scs->static_config.encoder_color_format = ((EbSvtAv1EncConfiguration*)config_struct)->encoder_color_format;

    scs->chroma_format_idc = (uint32_t)(scs->static_config.encoder_color_format);
    scs->encoder_bit_depth = (uint32_t)(scs->static_config.encoder_bit_depth);
    //16bit pipeline
    scs->is_16bit_pipeline = ((((EbSvtAv1EncConfiguration*)config_struct)->encoder_bit_depth) > EB_EIGHT_BIT) ? TRUE: FALSE;
    scs->subsampling_x = (scs->chroma_format_idc == EB_YUV444 ? 1 : 2) - 1;
    scs->subsampling_y = (scs->chroma_format_idc >= EB_YUV422 ? 1 : 2) - 1;
    scs->static_config.compressed_ten_bit_format = ((EbSvtAv1EncConfiguration*)config_struct)->compressed_ten_bit_format;

    // Thresholds
    scs->static_config.high_dynamic_range_input = ((EbSvtAv1EncConfiguration*)config_struct)->high_dynamic_range_input;
    scs->static_config.screen_content_mode = ((EbSvtAv1EncConfiguration*)config_struct)->screen_content_mode;
    // SC detection is OFF for first pass in M8
    uint8_t disable_sc_detection = (scs->static_config.pass == ENC_FIRST_PASS) ? 1 : 0;
    if (disable_sc_detection)
        scs->static_config.screen_content_mode = 0;

    // Annex A parameters
    scs->static_config.profile = ((EbSvtAv1EncConfiguration*)config_struct)->profile;
    scs->static_config.tier = ((EbSvtAv1EncConfiguration*)config_struct)->tier;
    scs->static_config.level = ((EbSvtAv1EncConfiguration*)config_struct)->level;
    scs->static_config.stat_report = ((EbSvtAv1EncConfiguration*)config_struct)->stat_report;

    // Buffers - Hardcoded(Cleanup)
    scs->static_config.use_cpu_flags = ((EbSvtAv1EncConfiguration*)config_struct)->use_cpu_flags;

    scs->static_config.channel_id = ((EbSvtAv1EncConfiguration*)config_struct)->channel_id;
    scs->static_config.active_channel_count = ((EbSvtAv1EncConfiguration*)config_struct)->active_channel_count;
    scs->static_config.logical_processors = ((EbSvtAv1EncConfiguration*)config_struct)->logical_processors;
    if (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) {
        if (scs->static_config.logical_processors == 0){
            scs->static_config.logical_processors = 3;
        }else if (scs->static_config.logical_processors > 3){
            scs->static_config.logical_processors = 3;
            SVT_WARN("lp is capped at 3 for low delay\n");
        }
    }
    scs->static_config.pin_threads = ((EbSvtAv1EncConfiguration*)config_struct)->pin_threads;
    scs->static_config.target_socket = ((EbSvtAv1EncConfiguration*)config_struct)->target_socket;
    if ((scs->static_config.pin_threads == 0) && (scs->static_config.target_socket != -1)){
        SVT_WARN("threads pinning 0 and ss %d is not a valid combination: unpin will be set to 0\n", scs->static_config.target_socket);
        scs->static_config.pin_threads = 1;
    }
    scs->static_config.qp = ((EbSvtAv1EncConfiguration*)config_struct)->qp;
    scs->static_config.recon_enabled = ((EbSvtAv1EncConfiguration*)config_struct)->recon_enabled;
    scs->static_config.enable_tpl_la = ((EbSvtAv1EncConfiguration*)config_struct)->enable_tpl_la;
    if (scs->static_config.enable_tpl_la != 1){
        scs->static_config.enable_tpl_la = 1;
    }
    // Extract frame rate from Numerator and Denominator if not 0
    if (scs->static_config.frame_rate_numerator != 0 && scs->static_config.frame_rate_denominator != 0)
        scs->frame_rate = ((scs->static_config.frame_rate_numerator << 8) / (scs->static_config.frame_rate_denominator)) << 8;
    // Get Default Intra Period if not specified
    if (scs->static_config.intra_period_length == -2)
        scs->static_config.intra_period_length = compute_default_intra_period(scs);
    else if (scs->static_config.multiply_keyint) {
        const double fps = (double)scs->static_config.frame_rate_numerator /
            scs->static_config.frame_rate_denominator;
        scs->static_config.intra_period_length =
            (int32_t)(fps * scs->static_config.intra_period_length);
    }
    if (scs->static_config.look_ahead_distance == (uint32_t)~0)
        scs->static_config.look_ahead_distance = compute_default_look_ahead(&scs->static_config);
    scs->static_config.enable_tf = config_struct->enable_tf;
    scs->static_config.enable_overlays = config_struct->enable_overlays;
    scs->static_config.superres_mode = config_struct->superres_mode;
    scs->static_config.superres_denom = config_struct->superres_denom;
    scs->static_config.superres_kf_denom = config_struct->superres_kf_denom;
    scs->static_config.superres_qthres = config_struct->superres_qthres;
    scs->static_config.superres_kf_qthres = config_struct->superres_kf_qthres;
    if (scs->static_config.superres_mode == SUPERRES_AUTO)
    {
        // TODO: set search mode based on preset
        //scs->static_config.superres_auto_search_type = SUPERRES_AUTO_SOLO;
        scs->static_config.superres_auto_search_type = SUPERRES_AUTO_DUAL;
        //scs->static_config.superres_auto_search_type = SUPERRES_AUTO_ALL;
    }

    scs->static_config.resize_mode = config_struct->resize_mode;
    scs->static_config.resize_denom = config_struct->resize_denom;
    scs->static_config.resize_kf_denom = config_struct->resize_kf_denom;
    if (config_struct->frame_scale_evts.start_frame_nums) {
        EB_NO_THROW_MALLOC(scs->static_config.frame_scale_evts.start_frame_nums, sizeof(int64_t) * config_struct->frame_scale_evts.evt_num);
        memcpy(scs->static_config.frame_scale_evts.start_frame_nums, config_struct->frame_scale_evts.start_frame_nums, sizeof(int64_t) * config_struct->frame_scale_evts.evt_num);
    }
    if (config_struct->frame_scale_evts.resize_kf_denoms) {
        EB_NO_THROW_MALLOC(scs->static_config.frame_scale_evts.resize_kf_denoms, sizeof(int32_t) * config_struct->frame_scale_evts.evt_num);
        memcpy(scs->static_config.frame_scale_evts.resize_kf_denoms, config_struct->frame_scale_evts.resize_kf_denoms, sizeof(int32_t) * config_struct->frame_scale_evts.evt_num);
    }
    if (config_struct->frame_scale_evts.resize_denoms) {
        EB_NO_THROW_MALLOC(scs->static_config.frame_scale_evts.resize_denoms, sizeof(int32_t) * config_struct->frame_scale_evts.evt_num);
        memcpy(scs->static_config.frame_scale_evts.resize_denoms, config_struct->frame_scale_evts.resize_denoms, sizeof(int32_t) * config_struct->frame_scale_evts.evt_num);
    }
    scs->static_config.frame_scale_evts.evt_num = config_struct->frame_scale_evts.evt_num;

    // Color description
    scs->static_config.color_description_present_flag = config_struct->color_description_present_flag;
    scs->static_config.color_primaries = config_struct->color_primaries;
    scs->static_config.transfer_characteristics = config_struct->transfer_characteristics;
    scs->static_config.matrix_coefficients = config_struct->matrix_coefficients;
    scs->static_config.color_range = config_struct->color_range;
    scs->static_config.chroma_sample_position = config_struct->chroma_sample_position;
    scs->static_config.mastering_display = config_struct->mastering_display;
    scs->static_config.content_light_level = config_struct->content_light_level;

    // switch frame
    scs->static_config.sframe_dist = config_struct->sframe_dist;
    scs->static_config.sframe_mode = config_struct->sframe_mode;
    scs->seq_header.max_frame_width = config_struct->forced_max_frame_width > 0 ? config_struct->forced_max_frame_width
        : scs->static_config.sframe_dist > 0 ? 16384 : scs->max_input_luma_width;
    scs->seq_header.max_frame_height = config_struct->forced_max_frame_height > 0 ? config_struct->forced_max_frame_height
        : scs->static_config.sframe_dist > 0 ? 8704 : scs->max_input_luma_height;
    scs->static_config.force_key_frames = config_struct->force_key_frames;

    // QM
    scs->static_config.enable_qm = config_struct->enable_qm;
    scs->static_config.min_qm_level = config_struct->min_qm_level;
    scs->static_config.max_qm_level = config_struct->max_qm_level;
    if (scs->static_config.enable_qm && scs->static_config.min_qm_level == 15 && scs->static_config.max_qm_level == 15) {
        SVT_WARN("Quantization matrices will be forced off since both min and max quant matrix levels are set to 15\n");
        scs->static_config.enable_qm = 0;
    }

#if FTR_STARTUP_MG_SIZE
    scs->static_config.startup_mg_size = config_struct->startup_mg_size;
#endif
    return;
}

/*
* return by-pass encdec
*/
uint8_t svt_aom_get_bypass_encdec(EncMode enc_mode, uint8_t hbd_md, uint8_t encoder_bit_depth) {
    UNUSED(hbd_md);
    uint8_t bypass_encdec = 1;
    if (encoder_bit_depth == EB_EIGHT_BIT) {
        // 8bit settings
        if (enc_mode <= ENC_M5)
            bypass_encdec = 0;
        else
            bypass_encdec = 1;
    } else {
        // 10bit settings
        // If the preset is changed for this feature, the warning in svt_av1_verify_settings in
        // EbEncSetting.c must also be changed to reflect the change in preset
        // This feature causes a mismatch between the files encoded for 10bit when bypass_encdec
        // is 1 if recon is enabled/disabled or stat-report is enabled/disabled
        if (enc_mode <= ENC_M9)
            bypass_encdec = 0;
        else
            bypass_encdec = 1;
    }
    return bypass_encdec;
}

// Get the nic_level used for each preset (to be passed to setting function: svt_aom_set_nic_controls())
// hierarchical_levels should be the sequence-level hierarchical structure (found in scs->static_config.hierarchical_levels
#if OPT_LD_M11
uint8_t svt_aom_get_nic_level(EncMode enc_mode, uint8_t is_base, uint8_t hierarchical_levels,
                              bool rtc_tune) {
#else
uint8_t svt_aom_get_nic_level(EncMode enc_mode, uint8_t is_base, uint8_t hierarchical_levels) {
#endif
    uint8_t nic_level;

    if (enc_mode <= ENC_MRS)
        nic_level = 0;
    else if (enc_mode <= ENC_MR)
        nic_level = 1;
    else if (enc_mode <= ENC_M0)
        nic_level = is_base ? 2 : 4;
    else if (enc_mode <= ENC_M1)
        nic_level = is_base ? 7 : 8;
    else if (enc_mode <= ENC_M3) {
        if (hierarchical_levels <= 3)
            nic_level = 10;
        else
            nic_level = 11;
    } else if (enc_mode <= ENC_M5)
        nic_level = 12;
    else if (enc_mode <= ENC_M6) {
        if (hierarchical_levels <= 3)
            nic_level = 15;
        else
            nic_level = 14;
    } else if (enc_mode <= ENC_M10)
        nic_level = 15;
    else if (enc_mode <= ENC_M11)
#if OPT_LD_M11
        if (rtc_tune)
            nic_level = is_base ? 15 : 16;
        else
            nic_level = hierarchical_levels <= 2 ? 16 : 15;
#else
        nic_level = hierarchical_levels <= 2 ? 16 : 15;
#endif
    else
        nic_level = 16;

    return nic_level;
}

// use this function to set the disallow_below_16x16 level in MD. ME 8x8 blocks are controlled by get_enable_me_8x8()
#if OPT_LD_M9
uint8_t svt_aom_get_disallow_below_16x16_picture_level(EncMode           enc_mode,
                                                       EbInputResolution resolution, Bool is_islice,
                                                       Bool sc_class1, Bool is_ref, bool rtc_tune) {
#else
uint8_t svt_aom_get_disallow_below_16x16_picture_level(EncMode enc_mode,
                                                       EbInputResolution resolution, Bool is_islice,
                                                       Bool sc_class1, Bool is_ref) {
#endif
    uint8_t disallow_below_16x16 = 0;
    if (sc_class1)
        disallow_below_16x16 = 0;
#if OPT_LD_M9
    else if ((enc_mode <= ENC_M8) || (rtc_tune && (enc_mode <= ENC_M9)))
#else
    else if (enc_mode <= ENC_M8)
#endif
        disallow_below_16x16 = 0;
    else if (enc_mode <= ENC_M12) {
        if (resolution <= INPUT_SIZE_1080p_RANGE)
            disallow_below_16x16 = is_ref ? 0 : 1;
        else
            disallow_below_16x16 = is_islice ? 0 : 1;
    } else
        disallow_below_16x16 = is_islice ? 0 : 1;

    return disallow_below_16x16;
}

/*
 * Generate per-SB MD settings (do not change per-PD)
 */
static EbErrorType signal_derivation_enc_dec_kernel_common(SequenceControlSet  *scs,
                                                           PictureControlSet   *pcs,
                                                           ModeDecisionContext *ctx) {
    EbErrorType return_error = EB_ErrorNone;

    EncMode enc_mode = pcs->enc_mode;

    SuperBlock *sb_ptr = pcs->sb_ptr_array[ctx->sb_index];
    set_detect_high_freq_ctrls(ctx, pcs->vq_ctrls.detect_high_freq_lvl);
    ctx->high_freq_present = 0;
#if OPT_LD_M9
    const bool rtc_tune = (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) ? true
                                                                                          : false;
#endif
    if (ctx->detect_high_freq_ctrls.enabled) {
        svt_aom_check_high_freq(pcs, sb_ptr, ctx);
    }

    // Level 0: pred depth only
    // Level 1: [-2, +2] depth refinement
    // Level 2: [-1, +1] depth refinement
    uint8_t depth_level = 0;
    if (enc_mode <= ENC_MRS)
        depth_level = 1;
    else if (pcs->ppcs->sc_class1) {
        if (enc_mode <= ENC_M1)
            depth_level = pcs->slice_type == I_SLICE ? 1 : 2;
        else if (enc_mode <= ENC_M4)
            depth_level = pcs->slice_type == I_SLICE ? 1 : 3;
        else if (enc_mode <= ENC_M8)
            depth_level = 3;
        else
            depth_level = pcs->slice_type == I_SLICE ? 3 : 0;
    }
#if OPT_LD_M9
    else if (rtc_tune) {
        if (enc_mode <= ENC_M0)
            depth_level = pcs->slice_type == I_SLICE ? 1 : 2;
        else if (enc_mode <= ENC_M4)
            depth_level = pcs->slice_type == I_SLICE ? 1 : 3;
        else if (enc_mode <= ENC_M8)
            depth_level = 3;
        else if (enc_mode <= ENC_M9) {
            depth_level = 3;
            if (pcs->slice_type != I_SLICE) {
                int me_8x8 = pcs->ppcs->me_8x8_cost_variance[ctx->sb_index];
                int th     = enc_mode <= ENC_M9 ? 60 * ctx->qp_index : 500 * ctx->qp_index;
                if (me_8x8 < th)
                    depth_level = 0;
            }
        } else if (enc_mode <= ENC_M10)
            depth_level = pcs->slice_type == I_SLICE ? 3 : 0;
        else
            depth_level = 0;
    }
#endif
    else if (enc_mode <= ENC_M0)
        depth_level = pcs->slice_type == I_SLICE ? 1 : 2;
    else if (enc_mode <= ENC_M4)
        depth_level = pcs->slice_type == I_SLICE ? 1 : 3;
    else if (enc_mode <= ENC_M8)
        depth_level = 3;
    else
        depth_level = 0;

    set_depth_ctrls(ctx, depth_level);

    // pic_pred_depth_only shouldn't be changed after this point
    ctx->pred_depth_only = ctx->pic_pred_depth_only = (depth_level == 0);

    set_lpd0_ctrls(ctx,
                   ctx->high_freq_present
                       ? MIN(pcs->pic_lpd0_lvl, ctx->detect_high_freq_ctrls.max_pic_lpd0_lvl)
                       : pcs->pic_lpd0_lvl);

    B64Geom *b64_geom                             = &pcs->ppcs->b64_geom[ctx->sb_index];
    ctx->depth_removal_ctrls.disallow_below_64x64 = 0;
    ctx->depth_removal_ctrls.disallow_below_32x32 = 0;
    /*
if disallow_below_16x16 is turned ON then enable_me_8x8 should be turned OFF for the same preset in order to save memory and cycles as that feature optimizes the me_candidate_array,
me_mv_array and the total_me_candidate_index arrays when 8x8 blocks are not used

if any check other than an I-SLICE check is used on disallow_below_16x16 then the enable_me_8x8 should be turned ON for the entire preset because without the 8x8 me data the non I-SLICE pictures
that use 8x8 blocks will lose significant BD-Rate as the parent 16x16 me data will be used for the 8x8 blocks
*/
    ctx->depth_removal_ctrls.disallow_below_16x16 = pcs->pic_disallow_below_16x16;

    if (b64_geom->width % 32 != 0 || b64_geom->height % 32 != 0)
        ctx->depth_removal_ctrls.disallow_below_64x64 = FALSE;
    if (b64_geom->width % 16 != 0 || b64_geom->height % 16 != 0)
        ctx->depth_removal_ctrls.disallow_below_32x32 = FALSE;
    if (b64_geom->width % 8 != 0 || b64_geom->height % 8 != 0)
        ctx->depth_removal_ctrls.disallow_below_16x16 = FALSE;
    ctx->disallow_4x4 = pcs->pic_disallow_4x4;

    // me_distortion/variance generated for 64x64 blocks only
    if (scs->super_block_size == 64) {
        set_depth_removal_level_controls(
            pcs,
            ctx,
            ctx->high_freq_present
                ? MAX(1,
                      (int)((int)pcs->pic_depth_removal_level -
                            (int)ctx->detect_high_freq_ctrls.depth_removal_shift))
                : pcs->pic_depth_removal_level);
    }
    if (/*scs->rc_stat_gen_pass_mode || */ ctx->skip_pd0) {
        // ctx->depth_removal_ctrls.disallow_below_64x64 = 1;
        ctx->depth_removal_ctrls.disallow_below_32x32 = 1;
        //ctx->depth_removal_ctrls.disallow_below_16x16 = 1;
        if (b64_geom->width % 32 != 0 || b64_geom->height % 32 != 0) {
            ctx->depth_removal_ctrls.disallow_below_64x64 = FALSE;
            ctx->depth_removal_ctrls.disallow_below_32x32 = FALSE;
        }
        if (b64_geom->width % 16 != 0 || b64_geom->height % 16 != 0)
            ctx->depth_removal_ctrls.disallow_below_32x32 = FALSE;
        else
            ctx->depth_removal_ctrls.enabled = 1;
        if (b64_geom->width % 8 != 0 || b64_geom->height % 8 != 0)
            ctx->depth_removal_ctrls.disallow_below_16x16 = FALSE;
    }

#if OPT_LD_M13
    if (rtc_tune && enc_mode > ENC_M8) {
        int lpd1_lvl = pcs->pic_lpd1_lvl;
        if (pcs->slice_type != I_SLICE) {
            int me_8x8 = pcs->ppcs->me_8x8_cost_variance[ctx->sb_index];
            int th     = enc_mode <= ENC_M12 ? 3 * ctx->qp_index : 6000;
            if (me_8x8 < th)
                lpd1_lvl += 2;
            else if (me_8x8 < 2 * th)
                lpd1_lvl += 1;
        }
        // Checking against 6, as 6 is the max level for lpd1_lvl. If max level for lpd1_lvl changes, the check should be updated
        lpd1_lvl = MAX(0, MIN(lpd1_lvl, 6));
        set_lpd1_ctrls(ctx, lpd1_lvl);
    } else
#endif
        set_lpd1_ctrls(ctx,
                       ctx->high_freq_present
                           ? MIN(pcs->pic_lpd1_lvl, ctx->detect_high_freq_ctrls.max_pic_lpd1_lvl)
                           : pcs->pic_lpd1_lvl);

    // 1st call to avoid using invalid settings at the construction of the block(s) queue of PD1 when regular PD0 is not called
    set_nsq_ctrls(ctx, pcs->nsq_level);
    return return_error;
}


void set_tx_shortcut_ctrls(PictureControlSet *pcs, ModeDecisionContext *ctx,
                           uint8_t tx_shortcut_level) {
    PictureParentControlSet *ppcs  = pcs->ppcs;
    TxShortcutCtrls         *ctrls = &ctx->tx_shortcut_ctrls;

    switch (tx_shortcut_level) {
    case 0:
        ctrls->bypass_tx_when_zcoeff = 0;
        ctrls->apply_pf_on_coeffs    = 0;
        ctrls->use_mds3_shortcuts_th = 0;
        ctrls->use_neighbour_info    = 0;
        ctrls->chroma_detector_level = 0;
        break;
    case 1:
        ctrls->bypass_tx_when_zcoeff = 1;
        ctrls->apply_pf_on_coeffs    = 0;
        ctrls->use_mds3_shortcuts_th = 0;
        ctrls->chroma_detector_level = 1;
        ctrls->use_neighbour_info    = 0;
        break;
    case 2:
        ctrls->bypass_tx_when_zcoeff = 1;
        ctrls->apply_pf_on_coeffs    = 1;
        ctrls->use_mds3_shortcuts_th = 0;
        ctrls->chroma_detector_level = 1;
        ctrls->use_neighbour_info    = 0;
        break;
    case 3:
        ctrls->bypass_tx_when_zcoeff = 1;
        ctrls->apply_pf_on_coeffs    = 1;
        ctrls->use_mds3_shortcuts_th = 25;
        ctrls->chroma_detector_level = 1;
        ctrls->use_neighbour_info    = 0;
        break;
    case 4:
        ctrls->bypass_tx_when_zcoeff = 1;
        ctrls->apply_pf_on_coeffs    = 1;
        ctrls->use_mds3_shortcuts_th = 25;
        ctrls->chroma_detector_level = ppcs->is_used_as_reference_flag ? 1 : 0;
        ctrls->use_neighbour_info    = 1;
        break;
    case 5:
        ctrls->bypass_tx_when_zcoeff = 1;
        ctrls->apply_pf_on_coeffs    = 1;
        ctrls->use_mds3_shortcuts_th = 50;
        ctrls->chroma_detector_level = 0;
        ctrls->use_neighbour_info    = 1;
        break;
    default: assert(0); break;
    }

    // Chroma detector should be used in M12 and below (at least in REF frames) to prevent blurring artifacts in some clips
    if (tx_shortcut_level && ppcs->is_used_as_reference_flag && pcs->enc_mode <= ENC_M12)
        assert(ctrls->chroma_detector_level &&
               "Chroma detector should be used for ref frames in low presets to prevent blurring "
               "artifacts.");
}


void signal_derivation_enc_dec_kernel_oq_light_pd1(PictureControlSet   *pcs,
                                                   ModeDecisionContext *ctx) {
    Pd1Level                 lpd1_level       = ctx->lpd1_ctrls.pd1_level;
    PictureParentControlSet *ppcs             = pcs->ppcs;
    const uint8_t            is_ref           = ppcs->is_used_as_reference_flag;
    const EbInputResolution  input_resolution = ppcs->input_resolution;
    const uint8_t            is_islice        = pcs->slice_type == I_SLICE;
    const SliceType          slice_type       = pcs->slice_type;
    // Get ref info, used to set some feature levels
    const uint32_t picture_qp           = pcs->picture_qp;
    uint32_t       me_8x8_cost_variance = (uint32_t)~0;
    uint32_t       me_64x64_distortion  = (uint32_t)~0;
    uint8_t        l0_was_skip = 0, l1_was_skip = 0;
    uint8_t        l0_was_64x64_mvp = 0, l1_was_64x64_mvp = 0;
#if OPT_LD_M13
    const bool rtc_tune = (pcs->scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B)
        ? true
        : false;
#endif
    // the frame size of reference pics are different if enable reference scaling.
    // sb info can not be reused because super blocks are mismatched, so we set
    // the reference pic unavailable to avoid using wrong info
    const Bool is_ref_l0_avail = is_ref_same_size(pcs, REF_LIST_0, 0);
    const Bool is_ref_l1_avail = is_ref_same_size(pcs, REF_LIST_1, 0);

    // REF info only available if frame is not an I_SLICE
    if (!is_islice && is_ref_l0_avail) {
        me_8x8_cost_variance = ppcs->me_8x8_cost_variance[ctx->sb_index];
        me_64x64_distortion  = ppcs->me_64x64_distortion[ctx->sb_index];
        EbReferenceObject *ref_obj_l0 =
            (EbReferenceObject *)pcs->ref_pic_ptr_array[REF_LIST_0][0]->object_ptr;
        l0_was_skip = ref_obj_l0->sb_skip[ctx->sb_index], l1_was_skip = 1;
        l0_was_64x64_mvp = ref_obj_l0->sb_64x64_mvp[ctx->sb_index], l1_was_64x64_mvp = 1;
        if (slice_type == B_SLICE && is_ref_l1_avail) {
            EbReferenceObject *ref_obj_l1 =
                (EbReferenceObject *)pcs->ref_pic_ptr_array[REF_LIST_1][0]->object_ptr;
            l1_was_skip      = ref_obj_l1->sb_skip[ctx->sb_index];
            l1_was_64x64_mvp = ref_obj_l1->sb_64x64_mvp[ctx->sb_index];
        }
    }
    uint8_t ref_skip_perc = pcs->ref_skip_percentage;

    // Set candidate reduction levels
    uint8_t cand_reduction_level = 0;
    if (is_islice)
        cand_reduction_level = 0;
    else if (lpd1_level <= LPD1_LVL_0)
        cand_reduction_level = 4;
    else if (lpd1_level <= LPD1_LVL_2)
        cand_reduction_level = 5;
    else if (lpd1_level <= LPD1_LVL_3)
        cand_reduction_level = 6;
    else
        cand_reduction_level = 7;
    if (ppcs->scs->rc_stat_gen_pass_mode)
        cand_reduction_level = 7;
    set_cand_reduction_ctrls(pcs,
                             ctx,
                             cand_reduction_level,
                             picture_qp,
                             me_8x8_cost_variance,
                             me_64x64_distortion,
                             l0_was_skip,
                             l1_was_skip,
                             ref_skip_perc);

    if (lpd1_level <= LPD1_LVL_0)
        ctx->rdoq_level = 4;
    else if (lpd1_level <= LPD1_LVL_4)
        ctx->rdoq_level = 5;
    else
        ctx->rdoq_level = 0;
    set_rdoq_controls(ctx, ctx->rdoq_level);
    if (lpd1_level <= LPD1_LVL_0)
        ctx->md_subpel_me_level = input_resolution <= INPUT_SIZE_1080p_RANGE ? (is_ref ? 9 : 11)
                                                                             : 15;
    else if (lpd1_level <= LPD1_LVL_1)
        ctx->md_subpel_me_level = input_resolution <= INPUT_SIZE_1080p_RANGE ? (is_ref ? 12 : 13)
                                                                             : 15;
    else if (lpd1_level <= LPD1_LVL_2) {
        ctx->md_subpel_me_level = input_resolution <= INPUT_SIZE_1080p_RANGE ? (is_ref ? 13 : 14)
                                                                             : 15;
    } else {
        ctx->md_subpel_me_level = input_resolution <= INPUT_SIZE_1080p_RANGE ? (is_ref ? 13 : 14)
                                                                             : 15;
        if (((l0_was_skip && l1_was_skip && ref_skip_perc > 50) ||
             (l0_was_64x64_mvp && l1_was_64x64_mvp)) &&
            me_8x8_cost_variance < (200 * picture_qp) && me_64x64_distortion < (200 * picture_qp))
            ctx->md_subpel_me_level = 0;
    }
    md_subpel_me_controls(ctx, ctx->md_subpel_me_level);

    uint8_t mds0_level = 0;
    if (lpd1_level <= LPD1_LVL_4)
        mds0_level = 4;
    else {
        mds0_level = is_ref ? 4 : 0;
        if (((l0_was_skip && l1_was_skip && ref_skip_perc > 40) ||
             (me_8x8_cost_variance < (250 * picture_qp) &&
              me_64x64_distortion < (250 * picture_qp))))
            mds0_level = 0;
    }
    set_mds0_controls(ctx, mds0_level);

    uint8_t lpd1_tx_level = 0;
    if (lpd1_level <= LPD1_LVL_2)
        lpd1_tx_level = 3;
    else {
        lpd1_tx_level = 4;
#if OPT_LD_M12_13
        if (!rtc_tune &&
            (((l0_was_skip && l1_was_skip && ref_skip_perc > 35) &&
              me_8x8_cost_variance < (800 * picture_qp) &&
              me_64x64_distortion < (800 * picture_qp)) ||
             (me_8x8_cost_variance < (100 * picture_qp) &&
              me_64x64_distortion < (100 * picture_qp))))
#else
        if (((l0_was_skip && l1_was_skip && ref_skip_perc > 35) &&
             me_8x8_cost_variance < (800 * picture_qp) &&
             me_64x64_distortion < (800 * picture_qp)) ||
            (me_8x8_cost_variance < (100 * picture_qp) && me_64x64_distortion < (100 * picture_qp)))
#endif
            lpd1_tx_level = 6;
    }
    set_lpd1_tx_ctrls(pcs, ctx, lpd1_tx_level);

    /* In modes below M13, only use level 1-3 for chroma detector, as more aggressive levels will cause
    blurring artifacts in certain clips.

    Do not test this signal in M12 and below during preset tuning.  This signal should be kept as an enc_mode check
    instead of and LPD1_LEVEL check to ensure that M12 and below do not use it.
    */
    if (pcs->enc_mode >= ENC_M13) {
        if (lpd1_level <= LPD1_LVL_4)
            ctx->lpd1_tx_ctrls.chroma_detector_level = 4;
        else
            ctx->lpd1_tx_ctrls.chroma_detector_level = 0;
    }

    /* In modes below M13, only skip non-NEAREST_NEAREST TX b/c skipping all inter TX will cause blocking artifacts
    in certain clips.  This signal is separated from the general lpd1_tx_ctrls (above) to avoid
    accidentally turning this on for modes below M13.

    Do not test this signal in M12 and below during preset tuning.  This signal should be kept as an enc_mode check
    instead of and LPD1_LEVEL check to ensure that M12 and below do not use it.
    */
#if OPT_LD_M13
    if (rtc_tune || pcs->enc_mode <= ENC_M12)
#else
    if (pcs->enc_mode <= ENC_M12)
#endif
        ctx->lpd1_skip_inter_tx_level = 0;
    else {
        assert(pcs->enc_mode >= ENC_M13 && "Only enable this feature for M13+");
        if (lpd1_level <= LPD1_LVL_2) {
            ctx->lpd1_skip_inter_tx_level = 0;
        } else if (lpd1_level <= LPD1_LVL_4) {
            ctx->lpd1_skip_inter_tx_level = 1;
            if (((l0_was_skip && l1_was_skip && ref_skip_perc > 35) &&
                 me_8x8_cost_variance < (800 * picture_qp) &&
                 me_64x64_distortion < (800 * picture_qp)) ||
                (me_8x8_cost_variance < (100 * picture_qp) &&
                 me_64x64_distortion < (100 * picture_qp))) {
                ctx->lpd1_skip_inter_tx_level = 2;
            }
        } else {
            ctx->lpd1_skip_inter_tx_level = is_ref ? 1 : 2;
            if (((l0_was_skip && l1_was_skip && ref_skip_perc > 35) &&
                 me_8x8_cost_variance < (800 * picture_qp) &&
                 me_64x64_distortion < (800 * picture_qp)) ||
                (me_8x8_cost_variance < (100 * picture_qp) &&
                 me_64x64_distortion < (100 * picture_qp))) {
                ctx->lpd1_skip_inter_tx_level = 2;
            }
        }
    }

    uint8_t rate_est_level = 0;
    if (lpd1_level <= LPD1_LVL_0)
        rate_est_level = 4;
    else
        rate_est_level = 0;
    set_rate_est_ctrls(ctx, rate_est_level);

    // If want to turn off approximating inter rate, must ensure that the approximation is also disabled
    // at the pic level (pcs->approx_inter_rate)
    ctx->approx_inter_rate = 1;

    uint8_t pf_level = 1;
    if (lpd1_level <= LPD1_LVL_4)
        pf_level = 1;
    else
        pf_level = 2;
    set_pf_controls(ctx, pf_level);

    uint8_t intra_level = 0;
    if (lpd1_level <= LPD1_LVL_2)
        intra_level = 4;
    else
        intra_level = 5;

    set_intra_ctrls(pcs, ctx, intra_level);

    /* Set signals that have assumed values in the light-PD1 path (but need to be initialized as they may be checked) */

    // Use coeff rate and slit flag rate only (i.e. no fast rate)
    ctx->shut_fast_rate             = FALSE;
    ctx->uv_ctrls.enabled           = 1;
    ctx->uv_ctrls.uv_mode           = CHROMA_MODE_1;
    ctx->uv_ctrls.nd_uv_serach_mode = 0;
    set_cfl_ctrls(ctx, 0);
    ctx->md_disallow_nsq                            = !ctx->nsq_ctrls.enabled;
    ctx->new_nearest_injection                      = 1;
    ctx->svt_aom_inject_inter_candidates            = 1;
    ctx->blk_skip_decision                          = TRUE;
    ctx->rate_est_ctrls.update_skip_ctx_dc_sign_ctx = 0;
    ctx->rate_est_ctrls.update_skip_coeff_ctx       = 0;
    ctx->subres_ctrls.odd_to_even_deviation_th      = 0;
}


EbErrorType svt_aom_signal_derivation_enc_dec_kernel_oq(SequenceControlSet  *scs,
                                                        PictureControlSet   *pcs,
                                                        ModeDecisionContext *ctx) {
    EbErrorType              return_error         = EB_ErrorNone;
    EncMode                  enc_mode             = pcs->enc_mode;
    uint8_t                  pd_pass              = ctx->pd_pass;
    PictureParentControlSet *ppcs                 = pcs->ppcs;
    const uint8_t            is_ref               = ppcs->is_used_as_reference_flag;
    const uint8_t            is_base              = ppcs->temporal_layer_index == 0;
    const EbInputResolution  input_resolution     = ppcs->input_resolution;
    const uint8_t            is_islice            = pcs->slice_type == I_SLICE;
    const uint32_t           hierarchical_levels  = scs->static_config.hierarchical_levels;
    const uint32_t           picture_qp           = pcs->picture_qp;
    uint32_t                 me_8x8_cost_variance = (uint32_t)~0;
    uint32_t                 me_64x64_distortion  = (uint32_t)~0;
    uint8_t                  l0_was_skip = 0, l1_was_skip = 0;
    uint8_t                  ref_skip_perc = pcs->ref_skip_percentage;

    // 2nd call as set_nsq_ctrls() has a PD_PASS check
    set_nsq_ctrls(ctx, pcs->nsq_level);

#if OPT_LD_M11
    const bool rtc_tune = (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) ? true
                                                                                          : false;
#endif

    set_cand_reduction_ctrls(pcs,
                             ctx,
                             pd_pass == PD_PASS_0 ? 0 : pcs->cand_reduction_level,
                             picture_qp,
                             me_8x8_cost_variance,
                             me_64x64_distortion,
                             l0_was_skip,
                             l1_was_skip,
                             ref_skip_perc);

    uint8_t txt_level = pcs->txt_level;
    if (ctx->high_freq_present)
        txt_level = txt_level == 0 ? ctx->detect_high_freq_ctrls.max_pd1_txt_lvl
                                   : MIN(txt_level, ctx->detect_high_freq_ctrls.max_pd1_txt_lvl);
    svt_aom_set_txt_controls(ctx, pd_pass == PD_PASS_0 ? 0 : txt_level);
    set_tx_shortcut_ctrls(pcs,
                          ctx,
                          pd_pass == PD_PASS_0         ? 0
                              : ctx->high_freq_present ? 0
                                                       : pcs->tx_shortcut_level);

    set_interpolation_search_level_ctrls(
        ctx, pd_pass == PD_PASS_0 ? 0 : pcs->interpolation_search_level);

    svt_aom_set_chroma_controls(ctx, pd_pass == PD_PASS_0 ? 0 : pcs->chroma_level);

    set_cfl_ctrls(ctx, pd_pass == PD_PASS_0 ? 0 : pcs->cfl_level);
    if (pd_pass == PD_PASS_0)
        ctx->md_disallow_nsq = enc_mode <= ENC_M0 ? !ctx->nsq_ctrls.enabled : 1;
    else {
        // Update nsq settings based on the sb_class
        ctx->md_disallow_nsq = !ctx->nsq_ctrls.enabled;
    }

    if (pd_pass == PD_PASS_0)
        ctx->global_mv_injection = 0;
    else
        ctx->global_mv_injection = ppcs->gm_ctrls.enabled;
    if (pd_pass == PD_PASS_0)
        ctx->new_nearest_injection = 0;
    else
        ctx->new_nearest_injection = 1;
    ctx->new_nearest_near_comb_injection = pd_pass == PD_PASS_0
        ? 0
        : pcs->new_nearest_near_comb_injection;

    //set Warped-Motion controls from Picture level.
    set_wm_controls(ctx, pd_pass == PD_PASS_0 ? 0 : pcs->wm_level);

    ctx->unipred3x3_injection            = pd_pass == PD_PASS_0 ? 0 : pcs->unipred3x3_injection;
    ctx->bipred3x3_injection             = pd_pass == PD_PASS_0 ? 0 : pcs->bipred3x3_injection;
    ctx->svt_aom_inject_inter_candidates = 1;
    ctx->inter_compound_mode             = pd_pass == PD_PASS_0 ? 0 : pcs->inter_compound_mode;
    svt_aom_set_dist_based_ref_pruning_controls(
        ctx, pd_pass == PD_PASS_0 ? 0 : pcs->dist_based_ref_pruning);
    set_spatial_sse_full_loop_level(ctx,
                                    pd_pass == PD_PASS_0 ? 0 : pcs->spatial_sse_full_loop_level);
    if (ctx->uv_ctrls.uv_mode <= CHROMA_MODE_1)
        ctx->blk_skip_decision = TRUE;
    else
        ctx->blk_skip_decision = FALSE;

#if OPT_LD_LATENCY_MD
    if (scs->low_latency_kf && is_islice)
        ctx->rdoq_level = 0;
    else
#endif
        if (pd_pass == PD_PASS_0) {
        if (enc_mode <= ENC_M3)
            ctx->rdoq_level = 1;
        else
            ctx->rdoq_level = 0;

    } else if (enc_mode <= ENC_M12)
        ctx->rdoq_level = 1;
#if OPT_LD_M13
    else if (rtc_tune)
        ctx->rdoq_level = 1;
#endif
    else
        ctx->rdoq_level = 5;
    set_rdoq_controls(ctx, ctx->rdoq_level);

    // Derive redundant block
    if (pd_pass == PD_PASS_0 || ctx->md_disallow_nsq)
        ctx->redundant_blk = FALSE;
    else
        ctx->redundant_blk = TRUE;
    set_parent_sq_coeff_area_based_cycles_reduction_ctrls(
        ctx, input_resolution, pd_pass == PD_PASS_0 ? 0 : ctx->nsq_ctrls.psq_cplx_lvl);
    ctx->md_depth_early_exit_th = (pd_pass == PD_PASS_0) ? 0 : pcs->pic_depth_early_exit_th;
    // Set pic_obmc_level @ MD
    if (pd_pass == PD_PASS_0)
        ctx->md_pic_obmc_level = 0;
    else
        ctx->md_pic_obmc_level = pcs->ppcs->pic_obmc_level;

    set_obmc_controls(ctx, ctx->md_pic_obmc_level);

    ctx->md_inter_intra_level = pcs->md_inter_intra_level;
    svt_aom_set_inter_intra_ctrls(ctx, pd_pass == PD_PASS_0 ? 0 : pcs->md_inter_intra_level);

    set_txs_controls(ctx, pd_pass == PD_PASS_0 ? 0 : pcs->txs_level);

    svt_aom_set_nic_controls(ctx, pd_pass == PD_PASS_0 ? 17 : pcs->nic_level);
    // Set md_filter_intra_mode @ MD
    // md_filter_intra_level specifies whether filter intra would be active
    // for a given prediction candidate in mode decision.

    // md_filter_intra_level | Settings
    // 0                      | OFF
    // 1                      | ON
    if (pd_pass == PD_PASS_0)
        ctx->md_filter_intra_level = 0;
    else
        ctx->md_filter_intra_level = pcs->pic_filter_intra_level;
    // Set md_allow_intrabc @ MD
    if (pd_pass == PD_PASS_0)
        ctx->md_allow_intrabc = 0;
    else
        ctx->md_allow_intrabc = pcs->ppcs->frm_hdr.allow_intrabc;

    // Set md_palette_level @ MD
    if (pd_pass == PD_PASS_0)
        ctx->md_palette_level = 0;
    else
        ctx->md_palette_level = pcs->ppcs->palette_level;

    uint8_t pf_level = 1;
    set_pf_controls(ctx, pf_level);
    md_sq_motion_search_controls(ctx, pd_pass == PD_PASS_0 ? 0 : pcs->md_sq_mv_search_level);

    md_nsq_motion_search_controls(ctx, pd_pass == PD_PASS_0 ? 0 : pcs->md_nsq_mv_search_level);

    svt_aom_md_pme_search_controls(ctx, pd_pass == PD_PASS_0 ? 0 : pcs->md_pme_level);
    if (pd_pass == PD_PASS_0)
        ctx->md_subpel_me_level = enc_mode <= ENC_M5 ? 3 : 0;
    else if (enc_mode <= ENC_M0)
        ctx->md_subpel_me_level = 1;
    else if (enc_mode <= ENC_M5)
        ctx->md_subpel_me_level = input_resolution <= INPUT_SIZE_480p_RANGE ? 1 : 2;
    else if (enc_mode <= ENC_M9)
        ctx->md_subpel_me_level = is_base ? 2 : (is_ref ? 4 : 7);
    else
        ctx->md_subpel_me_level = is_ref ? 4 : 7;
    ctx->md_subpel_me_level += hierarchical_levels <= 2 ? 2 : 0;
    ctx->md_subpel_me_level = MIN(15, ctx->md_subpel_me_level);
    md_subpel_me_controls(ctx, ctx->md_subpel_me_level);
    if (pd_pass == PD_PASS_0)
        ctx->md_subpel_pme_level = enc_mode <= ENC_M0 ? 3 : 0;
    else if (enc_mode <= ENC_M0)
        ctx->md_subpel_pme_level = 1;
    else
        ctx->md_subpel_pme_level = 2;

    md_subpel_pme_controls(ctx, ctx->md_subpel_pme_level);
    uint8_t rate_est_level = 0;
    if (pd_pass == PD_PASS_0) {
        if (enc_mode <= ENC_M2)
            rate_est_level = 1;
        else
            rate_est_level = 2;
    } else {
        if (enc_mode <= ENC_M10)
            rate_est_level = 1;
#if OPT_LD_M11
        else if (rtc_tune && enc_mode <= ENC_M12)
            rate_est_level = is_islice ? 1 : is_ref ? 3 : 4;
#endif
        else if (enc_mode <= ENC_M12)
            rate_est_level = is_islice ? 3 : 4;
        else
            rate_est_level = is_islice ? 3 : 0;
    }
    set_rate_est_ctrls(ctx, rate_est_level);

    // set at pic-level b/c feature depends on some pic-level initializations
    ctx->approx_inter_rate = pcs->approx_inter_rate;
    // Use coeff rate and slit flag rate only (i.e. no fast rate)
    if (pd_pass == PD_PASS_0)
        ctx->shut_fast_rate = TRUE;
    else
        ctx->shut_fast_rate = FALSE;

    // intra_level must be greater than 0 for I_SLICE
    uint8_t intra_level = 0;
    if (pd_pass == PD_PASS_0) {
        intra_level = 5;
    } else if (enc_mode <= ENC_M0)
        intra_level = 1;
    else if (enc_mode <= ENC_M1)
        intra_level = is_base ? 1 : 2;
    else if (enc_mode <= ENC_M4)
        intra_level = is_base ? 1 : 3;
    else if (enc_mode <= ENC_M5)
        intra_level = is_base ? 1 : 4;
    else if (enc_mode <= ENC_M8)
        intra_level = (is_islice || ppcs->transition_present == 1) ? 1 : is_base ? 2 : 4;
    else if (enc_mode <= ENC_M9)
        intra_level = (is_islice || ppcs->transition_present == 1) ? 1 : 4;
    else
        intra_level = (is_islice || ppcs->transition_present == 1) ? 3 : 4;
#if OPT_LD_LATENCY_MD
    if (pcs->scs->low_latency_kf && is_islice)
        intra_level = 4;
#endif
    set_intra_ctrls(pcs, ctx, intra_level);

    set_mds0_controls(ctx, pd_pass == PD_PASS_0 ? 2 : pcs->mds0_level);
    set_subres_controls(ctx, 0);
    ctx->inter_depth_bias = 0;
    uint8_t skip_4x4_depth_lvl;
    if (pd_pass == PD_PASS_0)
        skip_4x4_depth_lvl = 0;
    else if (enc_mode <= ENC_M0)
        skip_4x4_depth_lvl = 0;
    else
        skip_4x4_depth_lvl = 1;

    set_skip_4x4_depth_ctrls(&ctx->skip_4x4_depth_ctrls, skip_4x4_depth_lvl);
    return return_error;
}

/************************************************
 * Set ME/HME Params from Config
 ************************************************/
/************************************************
 * Set HME Search area parameters
 ************************************************/
void set_hme_search_params(PictureParentControlSet *pcs, MeContext *me_ctx,
                           EbInputResolution input_resolution) {
    // Set number of HME level 0 search regions to use
    me_ctx->num_hme_sa_w = 2;
    me_ctx->num_hme_sa_h = 2;

    // Set HME level 0 min and max search areas
    if (pcs->enc_mode <= ENC_MRS) {
        me_ctx->hme_l0_sa.sa_min = (SearchArea){ 240, 240 };
        me_ctx->hme_l0_sa.sa_max = (SearchArea){ 480, 480 };
    } else if (pcs->enc_mode <= ENC_M0) {
        if (input_resolution < INPUT_SIZE_4K_RANGE) {
            me_ctx->hme_l0_sa.sa_min = (SearchArea){32, 32};
            me_ctx->hme_l0_sa.sa_max = (SearchArea){192, 192};
        } else {
            me_ctx->hme_l0_sa.sa_min = (SearchArea){240, 240};
            me_ctx->hme_l0_sa.sa_max = (SearchArea){480, 480};
        }
    }
    else if (pcs->enc_mode <= ENC_M4) {
        me_ctx->hme_l0_sa.sa_min = (SearchArea){32, 32};
        me_ctx->hme_l0_sa.sa_max = (SearchArea){192, 192};
    }
    else if (pcs->enc_mode <= ENC_M6) {
        if (input_resolution <= INPUT_SIZE_1080p_RANGE) {
            if (pcs->sc_class1) {
                me_ctx->hme_l0_sa.sa_min = (SearchArea){32, 32};
                me_ctx->hme_l0_sa.sa_max = (SearchArea){192, 192};
            } else {
                me_ctx->hme_l0_sa.sa_min = (SearchArea) { 16, 16 };
                me_ctx->hme_l0_sa.sa_max = (SearchArea) { 192, 192 };
            }
        } else {
            me_ctx->hme_l0_sa.sa_min = (SearchArea){32, 32};
            me_ctx->hme_l0_sa.sa_max = (SearchArea){192, 192};
        }
    }

    else if (pcs->enc_mode <= ENC_M11) {
        if (pcs->sc_class1) {
            me_ctx->hme_l0_sa.sa_min = (SearchArea){32, 32};
            me_ctx->hme_l0_sa.sa_max = (SearchArea){192, 192};
        } else {
            me_ctx->hme_l0_sa.sa_min = (SearchArea) { 16, 16 };
            me_ctx->hme_l0_sa.sa_max = (SearchArea) { 192, 192 };
        }
    }
    else {
        if (pcs->sc_class1) {
            me_ctx->hme_l0_sa.sa_min = (SearchArea){ 32, 32 };
            me_ctx->hme_l0_sa.sa_max = (SearchArea){ 192, 192 };
        }
        else {
            if (input_resolution < INPUT_SIZE_4K_RANGE) {
                me_ctx->hme_l0_sa.sa_min = (SearchArea){ 8, 8 };
                me_ctx->hme_l0_sa.sa_max = (SearchArea){ 96, 96 };
            }
            else {
                me_ctx->hme_l0_sa.sa_min = (SearchArea){ 16, 16 };
                me_ctx->hme_l0_sa.sa_max = (SearchArea){ 96, 96 };
            }
        }
    }
    // Set the HME Level 1 and Level 2 refinement areas
    if (pcs->enc_mode <= ENC_M1) {
        me_ctx->hme_l1_sa = (SearchArea){16, 16};
        me_ctx->hme_l2_sa = (SearchArea){16, 16};
    } else {
        me_ctx->hme_l1_sa = (SearchArea){8, 3};
        me_ctx->hme_l2_sa = (SearchArea){8, 3};
    }
}

/************************************************
 * Set ME Search area parameters
 ************************************************/
void set_me_search_params(SequenceControlSet *scs, PictureParentControlSet *pcs,
                          MeContext *me_ctx, EbInputResolution input_resolution) {
#if FIX_LAYER_SIGNAL
    uint32_t hierarchical_levels = pcs->hierarchical_levels;
#else
    uint32_t hierarchical_levels = scs->static_config.hierarchical_levels;
#endif
    // Set the min and max ME search area
    if (pcs->sc_class1) {
        if (pcs->enc_mode <= ENC_M2) {
            me_ctx->me_sa.sa_min = (SearchArea){175, 175};
            me_ctx->me_sa.sa_max = (SearchArea){750, 750};
        } else if (pcs->enc_mode <= ENC_M4) {
            me_ctx->me_sa.sa_min = (SearchArea){125, 125};
            me_ctx->me_sa.sa_max = (SearchArea){500, 500};
        } else if (pcs->enc_mode <= ENC_M6) {
            me_ctx->me_sa.sa_min = (SearchArea){75, 75};
            me_ctx->me_sa.sa_max = (SearchArea){350, 350};
        } else if (pcs->enc_mode <= ENC_M7) {
            me_ctx->me_sa.sa_min = (SearchArea){48, 48};
            me_ctx->me_sa.sa_max = (SearchArea){224, 224};
        } else if (pcs->enc_mode <= ENC_M8) {
            me_ctx->me_sa.sa_min = (SearchArea){32, 32};
            me_ctx->me_sa.sa_max = (SearchArea){164, 164};
        } else if (pcs->enc_mode <= ENC_M10) {
            me_ctx->me_sa.sa_min = (SearchArea){32, 32};
            me_ctx->me_sa.sa_max = (SearchArea){96, 96};
        } else if (pcs->enc_mode <= ENC_M12) {
            me_ctx->me_sa.sa_min = (SearchArea) { 16, 16 };
            me_ctx->me_sa.sa_max = (SearchArea) { 96, 96 };
        }
        else {
            me_ctx->me_sa.sa_min = (SearchArea) { 8, 8 };
            me_ctx->me_sa.sa_max = (SearchArea) { 32, 32 };
        }
    } else if (pcs->enc_mode <= ENC_M0) {
        me_ctx->me_sa.sa_min = (SearchArea){64, 64};
        me_ctx->me_sa.sa_max = (SearchArea){256, 256};
    } else if (pcs->enc_mode <= ENC_M2) {
        me_ctx->me_sa.sa_min = (SearchArea){64, 64};
        me_ctx->me_sa.sa_max = (SearchArea){128, 128};
    }
    else if (pcs->enc_mode <= ENC_M3) {
        me_ctx->me_sa.sa_min = (SearchArea){16, 16};
        me_ctx->me_sa.sa_max = (SearchArea){128, 128};
    }
    else if (pcs->enc_mode <= ENC_M4) {
        me_ctx->me_sa.sa_min = (SearchArea){16, 16};
        me_ctx->me_sa.sa_max = (SearchArea){64, 64};
    }
    else if (pcs->enc_mode <= ENC_M5) {
        me_ctx->me_sa.sa_min = (SearchArea) { 16, 16 };
        me_ctx->me_sa.sa_max = (SearchArea) { 64, 32 };
    }
    else if (pcs->enc_mode <= ENC_M7) {
        if (input_resolution < INPUT_SIZE_1080p_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea) { 16, 16 };
            me_ctx->me_sa.sa_max = (SearchArea) { 32, 16 };
        }
        else {
            me_ctx->me_sa.sa_min = (SearchArea) { 16, 6 };
            me_ctx->me_sa.sa_max = (SearchArea) { 16, 9 };
        }
    }
    else if (pcs->enc_mode <= ENC_M8) {
        if (hierarchical_levels <= 3) {
            if (input_resolution < INPUT_SIZE_4K_RANGE) {
                me_ctx->me_sa.sa_min = (SearchArea) { 8, 5 };
                me_ctx->me_sa.sa_max = (SearchArea) { 16, 9 };
            }
            else {
                me_ctx->me_sa.sa_min = (SearchArea) { 8, 1 };
                me_ctx->me_sa.sa_max = (SearchArea) { 8, 1 };
            }
        }
        else if (input_resolution < INPUT_SIZE_1080p_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea) { 16, 16 };
            me_ctx->me_sa.sa_max = (SearchArea) { 32, 16 };
        }
        else {
            me_ctx->me_sa.sa_min = (SearchArea) { 16, 6 };
            me_ctx->me_sa.sa_max = (SearchArea) { 16, 9 };
        }
    }
    else if (pcs->enc_mode <= ENC_M9) {
        if (input_resolution < INPUT_SIZE_1080p_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea) { 16, 16 };
            me_ctx->me_sa.sa_max = (SearchArea) { 32, 16 };
        }
        else {
            me_ctx->me_sa.sa_min = (SearchArea) { 16, 6 };
            me_ctx->me_sa.sa_max = (SearchArea) { 16, 9 };
        }
    }
    else if (pcs->enc_mode <= ENC_M10) {
        if (input_resolution < INPUT_SIZE_4K_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea) { 8, 5 };
            me_ctx->me_sa.sa_max = (SearchArea) { 16, 9 };
        }
        else {
            me_ctx->me_sa.sa_min = (SearchArea) { 8, 1 };
            me_ctx->me_sa.sa_max = (SearchArea) { 8, 1 };
        }
    }
    else if (pcs->enc_mode <= ENC_M12) {
        if (input_resolution < INPUT_SIZE_720p_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea){8, 3};
            me_ctx->me_sa.sa_max = (SearchArea){16, 9};
        } else if (input_resolution < INPUT_SIZE_1080p_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea){8, 1};
            me_ctx->me_sa.sa_max = (SearchArea){16, 7};
        } else if (input_resolution < INPUT_SIZE_4K_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea){8, 1};
            me_ctx->me_sa.sa_max = (SearchArea){8, 7};
        } else {
            me_ctx->me_sa.sa_min = (SearchArea){8, 1};
            me_ctx->me_sa.sa_max = (SearchArea){8, 1};
        }
    } else {
        me_ctx->me_sa.sa_min = (SearchArea){8, 3};
        me_ctx->me_sa.sa_max = (SearchArea){8, 3};
    }
#if OPT_LD_M9
    const bool               rtc_tune = (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) ? true : false;
    if (rtc_tune && pcs->enc_mode > ENC_M8) {
        if (input_resolution < INPUT_SIZE_720p_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea) { 8, 3 };
            me_ctx->me_sa.sa_max = (SearchArea) { 16, 9 };
        }
        else if (input_resolution < INPUT_SIZE_1080p_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea) { 8, 1 };
            me_ctx->me_sa.sa_max = (SearchArea) { 16, 7 };
        }
        else if (input_resolution < INPUT_SIZE_4K_RANGE) {
            me_ctx->me_sa.sa_min = (SearchArea) { 8, 1 };
            me_ctx->me_sa.sa_max = (SearchArea) { 8, 7 };
        }
        else {
            me_ctx->me_sa.sa_min = (SearchArea) { 8, 1 };
            me_ctx->me_sa.sa_max = (SearchArea) { 8, 1 };
        }
    }
#endif
    // Scale up the MIN ME area if low frame rate
    bool low_frame_rate_flag = (scs->frame_rate >> 16);
    if (low_frame_rate_flag) {
        me_ctx->me_sa.sa_min.width  = (me_ctx->me_sa.sa_min.width * 3) >> 1;
        me_ctx->me_sa.sa_min.height = (me_ctx->me_sa.sa_min.height * 3) >> 1;
    }
}

/*set the skip_frame flag for ipp*/
void set_skip_frame_in_ipp(PictureParentControlSet * pcs,MeContext *me_ctx) {
    me_ctx->skip_frame = 0;
    int8_t max_heirachical_level = (int8_t)pcs->scs->max_heirachical_level;
    int8_t enc_mode = pcs->scs->static_config.enc_mode;
    int8_t pass = (int8_t)pcs->scs->static_config.pass;
    uint8_t skip_frame_first_pass = pcs->scs->ipp_pass_ctrls.skip_frame_first_pass;
    uint64_t  picture_number = pcs->picture_number;
    if (!skip_frame_first_pass) {
        if (enc_mode < ENC_M8)
            me_ctx->skip_frame = 0;
        else if (pass == ENC_SINGLE_PASS)
        {
            // SINGLE PASS
            if (max_heirachical_level <= 2)
                me_ctx->skip_frame = ((enc_mode >= ENC_M10) && (picture_number > 1 && picture_number % 2 > 0)) ? 1 : 0;
            else if ((enc_mode < ENC_M10 && (picture_number > 3 && picture_number % 4 > 0)) ||
                (enc_mode >= ENC_M10 && (picture_number > 5 && picture_number % 6 > 0))) {
                me_ctx->skip_frame = 1;
            }
        }
    }
    else {
        if ((skip_frame_first_pass == 1) &&
            (picture_number % 8 > 0))
            me_ctx->skip_frame = 1;
        else if ((skip_frame_first_pass == 2) &&
            (picture_number > 7 && picture_number % 8 > 0))
            me_ctx->skip_frame = 1;
        else
            if (picture_number > 3 && picture_number % 4 > 0)
                me_ctx->skip_frame = 1;
            else
                me_ctx->skip_frame = 0;
    }
}

/******************************************************
* Derive ME Settings for OQ
  Input   : encoder mode and tune
  Output  : ME Kernel signal(s)
******************************************************/
EbErrorType svt_aom_signal_derivation_me_kernel_oq(SequenceControlSet *       scs,
                                           PictureParentControlSet *  pcs,
                                           MotionEstimationContext_t *me_context_ptr) {
    EbErrorType return_error = EB_ErrorNone;

    EncMode         enc_mode         = pcs->enc_mode;
    EbInputResolution input_resolution = scs->input_resolution;
#if FIX_LAYER_SIGNAL
    const uint32_t hierarchical_levels = pcs->hierarchical_levels;
#else
    const uint32_t hierarchical_levels = scs->static_config.hierarchical_levels;
#endif
    // Set ME search area
    set_me_search_params(scs, pcs, me_context_ptr->me_ctx, input_resolution);

    // Set HME search area
    set_hme_search_params(pcs, me_context_ptr->me_ctx, input_resolution);

    // Set HME flags
    me_context_ptr->me_ctx->enable_hme_flag        = pcs->enable_hme_flag;
    me_context_ptr->me_ctx->enable_hme_level0_flag = pcs->enable_hme_level0_flag;
    me_context_ptr->me_ctx->enable_hme_level1_flag = pcs->enable_hme_level1_flag;
    me_context_ptr->me_ctx->enable_hme_level2_flag = pcs->enable_hme_level2_flag;
    // HME Search Method
    me_context_ptr->me_ctx->hme_search_method = SUB_SAD_SEARCH;
    me_context_ptr->me_ctx->me_search_method  = SUB_SAD_SEARCH;


    if (pcs->sc_class1) {
        me_context_ptr->me_ctx->reduce_hme_l0_sr_th_min = 0;
        me_context_ptr->me_ctx->reduce_hme_l0_sr_th_max = 0;
    }
    else if (pcs->enc_mode <= ENC_M10) {
        me_context_ptr->me_ctx->reduce_hme_l0_sr_th_min = 0;
        me_context_ptr->me_ctx->reduce_hme_l0_sr_th_max = 0;
    }
    else {
        me_context_ptr->me_ctx->reduce_hme_l0_sr_th_min = 8;
        me_context_ptr->me_ctx->reduce_hme_l0_sr_th_max = 200;
    }
    // Set pre-hme level (0-2)
    uint8_t prehme_level = 0;
    if (pcs->sc_class1)
        prehme_level = 1;
    else
    {
        if (enc_mode <= ENC_M10)
            prehme_level = 1;
        else
            prehme_level = 3;
    }
    if (pcs->enable_hme_level1_flag == 0)
        prehme_level = 0;

    svt_aom_set_prehme_ctrls(me_context_ptr->me_ctx, prehme_level);

    // Set hme/me based reference pruning level (0-4)
    if (pcs->sc_class1) {
        if (enc_mode <= ENC_MRS)
            svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 0);
        else if (enc_mode <= ENC_M1)
            svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 2);
        else if (enc_mode <= ENC_M9)
            svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 5);
        else
            svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 6);
    } else {
        if (enc_mode <= ENC_MRS)
            svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 0);
        else if (enc_mode <= ENC_M0)
            svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 2);
        else if (enc_mode <= ENC_M1) {
            if (pcs->temporal_layer_index == 0)
                svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 2);
            else
                svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 4);
        }
        else if (enc_mode <= ENC_M7) {
            if (pcs->temporal_layer_index == 0)
                svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 2);
            else
                svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 5);
        }
        else
            svt_aom_set_me_hme_ref_prune_ctrls(me_context_ptr->me_ctx, 5);
    }
    // Set hme-based me sr adjustment level
    if (pcs->sc_class1)
        if (enc_mode <= ENC_M9)
            svt_aom_set_me_sr_adjustment_ctrls(me_context_ptr->me_ctx, 4);
        else
            svt_aom_set_me_sr_adjustment_ctrls(me_context_ptr->me_ctx, 5);
    else if (enc_mode <= ENC_MRS)
        svt_aom_set_me_sr_adjustment_ctrls(me_context_ptr->me_ctx, 0);
    else if (enc_mode <= ENC_M2)
        svt_aom_set_me_sr_adjustment_ctrls(me_context_ptr->me_ctx, 1);
    else
        svt_aom_set_me_sr_adjustment_ctrls(me_context_ptr->me_ctx, 3);
    if (enc_mode <= ENC_M2)
        me_context_ptr->me_ctx->prune_me_candidates_th = 0;
    else
        me_context_ptr->me_ctx->prune_me_candidates_th = 65;
    // Set signal at picture level b/c may check signal in MD
    me_context_ptr->me_ctx->use_best_unipred_cand_only =
        pcs->use_best_me_unipred_cand_only;
    if (enc_mode <= ENC_M6)
        me_context_ptr->me_ctx->me_early_exit_th = 0;
    else if (enc_mode <= ENC_M7) {
        if (hierarchical_levels <= 3)
            me_context_ptr->me_ctx->me_early_exit_th = BLOCK_SIZE_64 * BLOCK_SIZE_64 * 8;
        else
            me_context_ptr->me_ctx->me_early_exit_th = 0;
    }
    else if (pcs->enc_mode <= ENC_M10)
        me_context_ptr->me_ctx->me_early_exit_th = BLOCK_SIZE_64 * BLOCK_SIZE_64 * 8;
    else
        me_context_ptr->me_ctx->me_early_exit_th = BLOCK_SIZE_64 * BLOCK_SIZE_64 * 9;

    me_context_ptr->me_ctx->skip_frame = 0;
    return return_error;
};

/******************************************
* Verify Settings
******************************************/
EbErrorType svt_av1_verify_settings(SequenceControlSet *scs) {
    EbErrorType               return_error   = EB_ErrorNone;
    EbSvtAv1EncConfiguration *config         = &scs->static_config;
    unsigned int              channel_number = config->channel_id;
    if (config->enc_mode > MAX_ENC_PRESET) {
        SVT_ERROR("Instance %u: EncoderMode must be in the range of [0-%d]\n",
                  channel_number + 1,
                  MAX_ENC_PRESET);
        return_error = EB_ErrorBadParameter;
    }
    if (config->enc_mode == MAX_ENC_PRESET) {
        SVT_WARN(
            "EncoderMode (preset): %d was developed for the sole purpose of debugging and or "
            "running fast convex-hull encoding. This configuration should not be used for any "
            "benchmarking or quality analysis\n",
            MAX_ENC_PRESET);
    }
    if (scs->max_input_luma_width < 64) {
        SVT_ERROR("Instance %u: Source Width must be at least 64\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (scs->max_input_luma_height < 64) {
        SVT_ERROR("Instance %u: Source Height must be at least 64\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (scs->max_input_luma_width % 2) {
        SVT_ERROR("Error Instance %u: Source Width must be even for YUV_420 colorspace\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (scs->max_input_luma_height % 2) {
        SVT_ERROR("Error Instance %u: Source Height must be even for YUV_420 colorspace\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->pred_structure > 2 || config->pred_structure < 1) {
        SVT_ERROR("Instance %u: Pred Structure must be [1 or 2]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->pred_structure == 1 && config->pass > 0) {
        SVT_ERROR("Instance %u: Multi-passes is not support with Low Delay mode \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->vbr_bias_pct > 100) {
        SVT_ERROR("Instance %u: The bias percentage must be [0, 100]  \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->maximum_buffer_size_ms < 20 || config->maximum_buffer_size_ms > 10000) {
        SVT_ERROR("Instance %u: The maximum buffer size must be between [20, 10000]\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->starting_buffer_level_ms < 20 || config->starting_buffer_level_ms > 10000) {
        SVT_ERROR("Instance %u: The initial buffer size must be between [20, 10000] \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    } else if (config->starting_buffer_level_ms >= config->maximum_buffer_size_ms) {
        SVT_WARN(
            "The initial buffer size must be less than maximum buffer size. Defaulting optimal "
            "buffer size to maximum buffer size - 1 (%u)\n",
            config->maximum_buffer_size_ms - 1);
        config->starting_buffer_level_ms = (config->maximum_buffer_size_ms - 1);
    }

    if (config->optimal_buffer_level_ms < 20 || config->optimal_buffer_level_ms > 10000) {
        SVT_ERROR("Instance %u: The optimal buffer size must be between [20, 10000]\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    } else if (config->optimal_buffer_level_ms >= config->maximum_buffer_size_ms) {
        SVT_WARN(
            "The optimal buffer size must be less than maximum buffer size. Defaulting optimal "
            "buffer size to maximum buffer size - 1 (%u)\n",
            config->maximum_buffer_size_ms - 1);
        config->optimal_buffer_level_ms = (config->maximum_buffer_size_ms - 1);
    }

    if (config->over_shoot_pct > 100) {
        SVT_ERROR("Instance %u: The overshoot percentage must be between [0, 100] \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->mbr_over_shoot_pct > 100) {
        SVT_ERROR("Instance %u: The max bitrate overshoot percentage must be between [0, 100] \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->under_shoot_pct > 100) {
        SVT_ERROR("Instance %u: The undershoot percentage must be between [0, 100] \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->target_bit_rate > 100000000) {
        SVT_ERROR("Instance %u: The target bit rate must be between [0, 100000] kbps \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->max_bit_rate > 100000000) {
        SVT_ERROR("Instance %u: The maximum bit rate must be between [0, 100000] kbps \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->vbr_max_section_pct > 10000) {
        SVT_ERROR("Instance %u: The max section percentage must be between [0, 10000] \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->vbr_min_section_pct > 100) {
        SVT_ERROR("Instance %u: The min section percentage must be between [0, 100] \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->gop_constraint_rc &&
        ((config->rate_control_mode != SVT_AV1_RC_MODE_VBR) || config->intra_period_length < 119)) {
        SVT_ERROR(
            "Instance %u: Gop constraint rc is only supported with VBR mode when Gop size is "
            "greater than 119 \n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->gop_constraint_rc > 1) {
        SVT_ERROR("Instance %u: Invalid gop_constraint_rc. gop_constraint_rc must be [0 - 1]\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->gop_constraint_rc)
        SVT_WARN(
            "Instance %u: The GoP constraint RC mode is a work-in-progress project, and is only "
            "available for demos, experimentation, and further development uses and should not be "
            "used for benchmarking until fully implemented.\n",
            channel_number + 1);
    if (config->force_key_frames &&
        (config->rate_control_mode != SVT_AV1_RC_MODE_CQP_OR_CRF ||
         config->pred_structure != SVT_AV1_PRED_RANDOM_ACCESS)) {
        SVT_ERROR("Instance %u: Force key frame is only supported with RA CRF/CQP mode\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->rate_control_mode != SVT_AV1_RC_MODE_CQP_OR_CRF &&
        (config->target_bit_rate >= config->max_bit_rate) && (config->max_bit_rate != 0)) {
        SVT_ERROR("Instance %u: Max Bitrate must be greater than Target Bitrate\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (scs->max_input_luma_width > 16384) {
        SVT_ERROR("Instance %u: Source Width must be less than or equal to 16384\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (scs->max_input_luma_height > 8704) {
        SVT_ERROR("Instance %u: Source Height must be less than or equal to 8704)\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (scs->seq_header.max_frame_width < 64) {
        SVT_ERROR("Instance %u: Forced Max Width must be at least 64\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (scs->seq_header.max_frame_height < 64) {
        SVT_ERROR("Instance %u: Forced Max Height must be at least 64\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (scs->seq_header.max_frame_width > 16384) {
        SVT_ERROR("Instance %u: Forced Max Width must be less than or equal to 16384\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (scs->seq_header.max_frame_height > 8704) {
        SVT_ERROR("Instance %u: Forced Max Height must be less than or equal to 8704)\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if ((scs->max_input_luma_width > scs->seq_header.max_frame_width) ||
        (scs->max_input_luma_height > scs->seq_header.max_frame_height)) {
        SVT_ERROR(
            "Error instance %u: Source Width/Height must be less than or equal to Forced Max "
            "Width/Height\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->level != 0 && (config->level < 20 || config->level > 73)) {
        SVT_ERROR("Instance %u: Level must be in the range of [2.0-7.3]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->qp > MAX_QP_VALUE) {
        SVT_ERROR("Instance %u: %s must be [0 - %d]\n",
                  channel_number + 1,
                  config->enable_adaptive_quantization ? "CRF" : "QP",
                  MAX_QP_VALUE);
        return_error = EB_ErrorBadParameter;
    }
    if (config->hierarchical_levels > 5) {
        SVT_ERROR("Instance %u: Hierarchical Levels supported [0-5]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if ((config->intra_period_length < -2 || config->intra_period_length > 2 * ((1 << 30) - 1)) &&
        config->rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF) {
        SVT_ERROR("Instance %u: The intra period must be [-2, 2^31-2]  \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if ((config->intra_period_length < 0) && config->rate_control_mode == SVT_AV1_RC_MODE_VBR) {
        SVT_ERROR("Instance %u: The intra period must be > 0 for RateControlMode %d \n",
                  channel_number + 1,
                  config->rate_control_mode);
        return_error = EB_ErrorBadParameter;
    }

    if (config->intra_refresh_type > 2 || config->intra_refresh_type < 1) {
        SVT_ERROR("Instance %u: Invalid intra Refresh Type [1-2]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->enable_dlf_flag > 1) {
        SVT_ERROR("Instance %u: Invalid LoopFilterEnable. LoopFilterEnable must be [0 - 1]\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->rate_control_mode > SVT_AV1_RC_MODE_CBR &&
        (config->pass == ENC_FIRST_PASS || config->rc_stats_buffer.buf)) {
        SVT_ERROR("Instance %u: Only rate control mode 0~2 are supported for 2-pass \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->profile > 2) {
        SVT_ERROR("Instance %u: The maximum allowed profile value is 2 \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    // Check if the current input video is conformant with the Level constraint
    if (scs->frame_rate > (240 << 16)) {
        SVT_ERROR("Instance %u: The maximum allowed frame rate is 240 fps\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    // Check that the frame_rate is non-zero
    if (!scs->frame_rate) {
        SVT_ERROR("Instance %u: The frame rate should be greater than 0 fps \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (scs->static_config.frame_rate_numerator == 0 ||
        scs->static_config.frame_rate_denominator == 0) {
        SVT_ERROR(
            "Instance %u: The frame_rate_numerator and frame_rate_denominator must be greater than "
            "0\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->recode_loop > 4) {
        SVT_ERROR("Instance %u: The recode_loop must be [0 - 4] \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->rate_control_mode > SVT_AV1_RC_MODE_CBR) {
        SVT_ERROR("Instance %u: The rate control mode must be [0 - 2] \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->look_ahead_distance > MAX_LAD && config->look_ahead_distance != (uint32_t)~0) {
        SVT_ERROR(
            "Instance %u: The lookahead distance must be [0 - %d] \n", channel_number + 1, MAX_LAD);

        return_error = EB_ErrorBadParameter;
    }
    if ((unsigned)config->tile_rows > 6 || (unsigned)config->tile_columns > 6) {
        SVT_ERROR("Instance %u: Log2Tile rows/cols must be [0 - 6] \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if ((1u << config->tile_rows) * (1u << config->tile_columns) > 128 ||
        config->tile_columns > 4) {
        SVT_ERROR("Instance %u: MaxTiles is 128 and MaxTileCols is 16 (Annex A.3) \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->restricted_motion_vector > 1) {
        SVT_ERROR("Instance %u : Invalid Restricted Motion Vector flag [0 - 1]\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->max_qp_allowed > MAX_QP_VALUE) {
        SVT_ERROR("Instance %u: MaxQpAllowed must be [1 - %d]\n", channel_number + 1, MAX_QP_VALUE);
        return_error = EB_ErrorBadParameter;
    } else if (config->min_qp_allowed >= MAX_QP_VALUE) {
        SVT_ERROR(
            "Instance %u: MinQpAllowed must be [1 - %d]\n", channel_number + 1, MAX_QP_VALUE - 1);
        return_error = EB_ErrorBadParameter;
    } else if ((config->min_qp_allowed) > (config->max_qp_allowed)) {
        SVT_ERROR("Instance %u:  MinQpAllowed must be smaller than MaxQpAllowed\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    } else if ((config->min_qp_allowed) == 0) {
        SVT_ERROR("Instance %u: MinQpAllowed must be [1 - %d]. Lossless coding not supported\n",
                  channel_number + 1,
                  MAX_QP_VALUE - 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->use_qp_file > 1) {
        SVT_ERROR("Instance %u : Invalid use_qp_file. use_qp_file must be [0 - 1]\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->use_fixed_qindex_offsets > 2) {
        SVT_ERROR("Instance %u: The use_fixed_qindex_offsets must be [0 - 2] \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->key_frame_qindex_offset < -64 || config->key_frame_qindex_offset > 63) {
        SVT_ERROR(
            "Instance %u : Invalid key_frame_qindex_offset. key_frame_qindex_offset must be [-64 - "
            "63]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    for (uint8_t i = 0; i < config->hierarchical_levels + 1; ++i) {
        if (config->qindex_offsets[i] < -64 || config->qindex_offsets[i] > 63) {
            SVT_ERROR("Instance %u : Invalid qindex_offsets. qindex_offsets must be [-64 - 63]\n",
                      channel_number + 1);
            return_error = EB_ErrorBadParameter;
        }
    }
    if (config->key_frame_chroma_qindex_offset < -64 ||
        config->key_frame_chroma_qindex_offset > 63) {
        SVT_ERROR(
            "Instance %u : Invalid key_frame_chroma_qindex_offset. key_frame_chroma_qindex_offset "
            "must be [-64 - 63]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->luma_y_dc_qindex_offset < -64 || config->luma_y_dc_qindex_offset > 63) {
        SVT_ERROR(
            "Instance %u : Invalid luma_y_dc_qindex_offset. luma_y_dc_qindex_offset "
            "must be [-64 - 63]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->chroma_u_dc_qindex_offset < -64 || config->chroma_u_dc_qindex_offset > 63) {
        SVT_ERROR(
            "Instance %u : Invalid chroma_u_dc_qindex_offset. chroma_u_dc_qindex_offset "
            "must be [-64 - 63]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->chroma_u_ac_qindex_offset < -64 || config->chroma_u_ac_qindex_offset > 63) {
        SVT_ERROR(
            "Instance %u : Invalid chroma_u_ac_qindex_offset. chroma_u_ac_qindex_offset "
            "must be [-64 - 63]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->chroma_v_dc_qindex_offset < -64 || config->chroma_v_dc_qindex_offset > 63) {
        SVT_ERROR(
            "Instance %u : Invalid chroma_v_dc_qindex_offset. chroma_v_dc_qindex_offset "
            "must be [-64 - 63]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->chroma_v_ac_qindex_offset < -64 || config->chroma_v_ac_qindex_offset > 63) {
        SVT_ERROR(
            "Instance %u : Invalid chroma_v_ac_qindex_offset. chroma_v_ac_qindex_offset "
            "must be [-64 - 63]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    for (uint8_t i = 0; i < config->hierarchical_levels + 1; ++i) {
        if (config->chroma_qindex_offsets[i] < -64 || config->chroma_qindex_offsets[i] > 63) {
            SVT_ERROR(
                "Instance %u : Invalid chroma_qindex_offsets. chroma_qindex_offsets must be [-64 - "
                "63]\n",
                channel_number + 1);
            return_error = EB_ErrorBadParameter;
        }
    }

    if (config->stat_report == 1) {
        SVT_WARN("Instances %u: Enabling StatReport can decrease encoding speed\n",
                 channel_number + 1);
    }

    if (config->stat_report > 1) {
        SVT_ERROR("Instance %u : Invalid StatReport. StatReport must be [0 - 1]\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->high_dynamic_range_input > 1) {
        SVT_ERROR(
            "Instance %u : Invalid HighDynamicRangeInput. HighDynamicRangeInput must be [0 - 1]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->screen_content_mode > 2) {
        SVT_ERROR(
            "Instance %u : Invalid screen_content_mode. screen_content_mode must be [0 - 2]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    // IntraBC
    if (scs->intrabc_mode > 3 || scs->intrabc_mode < -1) {
        SVT_ERROR("Instance %u: Invalid intraBC mode [0-3, -1 for default], your input: %i\n",
                  channel_number + 1,
                  scs->intrabc_mode);
        return_error = EB_ErrorBadParameter;
    }

    if (scs->intrabc_mode > 0 && config->screen_content_mode != 1) {
        SVT_ERROR(
            "Instance %u: The intra BC feature is only available when screen_content_mode is set "
            "to 1\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (scs->static_config.enable_adaptive_quantization > 2) {
        SVT_ERROR(
            "Instance %u : Invalid enable_adaptive_quantization. enable_adaptive_quantization must "
            "be [0-2]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if ((config->encoder_bit_depth != 8) && (config->encoder_bit_depth != 10)) {
        SVT_ERROR("Instance %u: Encoder Bit Depth shall be only 8 or 10 \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    // Check if the EncoderBitDepth is conformant with the Profile constraint
    if ((config->profile == 0 || config->profile == 1) && config->encoder_bit_depth > 10) {
        SVT_ERROR(
            "Instance %u: The encoder bit depth shall be equal to 8 or 10 for Main/High Profile\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->encoder_color_format != EB_YUV420) {
        SVT_ERROR("Instance %u: Only support 420 now \n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->profile == 0 && config->encoder_color_format > EB_YUV420) {
        SVT_ERROR("Instance %u: Non 420 color format requires profile 1 or 2\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->profile == 1 && config->encoder_color_format != EB_YUV444) {
        SVT_ERROR("Instance %u: Profile 1 requires 4:4:4 color format\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->profile == 2 && config->encoder_bit_depth <= 10 &&
        config->encoder_color_format != EB_YUV422) {
        SVT_ERROR("Instance %u: Profile 2 bit-depth < 10 requires 4:2:2 color format\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->compressed_ten_bit_format != 0 && config->compressed_ten_bit_format != 1) {
        SVT_ERROR("Instance %u: Invalid Compressed Ten Bit Format flag [0 - 1]\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->compressed_ten_bit_format) {
        SVT_WARN("Instance %u: Compressed Ten Bit Format has been deprecated and has no effect\n",
                 channel_number + 1);
    }

    if (config->use_cpu_flags & EB_CPU_FLAGS_INVALID) {
        SVT_ERROR(
            "Instance %u: param '--asm' have invalid value.\n"
            "Value should be [0 - 11] or [c, mmx, sse, sse2, sse3, ssse3, sse4_1, sse4_2, avx, "
            "avx2, avx512, max]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->target_socket != -1 && config->target_socket != 0 && config->target_socket != 1) {
        SVT_ERROR("Instance %u: Invalid target_socket. target_socket must be [-1 - 1] \n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    // HBD mode decision
    if (scs->enable_hbd_mode_decision < (int8_t)(-1) || scs->enable_hbd_mode_decision > 2) {
        SVT_ERROR("Instance %u: Invalid HBD mode decision flag [-1 - 2], your input: %d\n",
                  channel_number + 1,
                  scs->enable_hbd_mode_decision);
        return_error = EB_ErrorBadParameter;
    }

    // CDEF
    if (config->cdef_level > 4 || config->cdef_level < -1) {
        SVT_ERROR("Instance %u: Invalid CDEF level [0 - 4, -1 for auto], your input: %d\n",
                  channel_number + 1,
                  config->cdef_level);
        return_error = EB_ErrorBadParameter;
    }

    // Restoration Filtering
    if (config->enable_restoration_filtering != 0 && config->enable_restoration_filtering != 1 &&
        config->enable_restoration_filtering != -1) {
        SVT_ERROR("Instance %u: Invalid restoration flag [0 - 1, -1 for auto], your input: %d\n",
                  channel_number + 1,
                  config->enable_restoration_filtering);
        return_error = EB_ErrorBadParameter;
    }

    if (config->enable_mfmv != 0 && config->enable_mfmv != 1 && config->enable_mfmv != -1) {
        SVT_ERROR(
            "Instance %u: Invalid motion field motion vector flag [0/1 or -1 for auto], your "
            "input: %d\n",
            channel_number + 1,
            config->enable_mfmv);
        return_error = EB_ErrorBadParameter;
    }

    if (config->enable_dg > 1) {
        SVT_ERROR(
            "Instance %u: Invalid dynamic GoP flag [0 - 1], your "
            "input: %d\n",
            channel_number + 1,
            config->enable_dg);
        return_error = EB_ErrorBadParameter;
    }

    if (config->fast_decode > 1) {
        SVT_ERROR(
            "Instance %u: Invalid fast decode flag [0 - 1, 0 for no decoder optimization], your "
            "input: %d\n",
            channel_number + 1,
            config->fast_decode);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tune > 1) {
        SVT_ERROR(
            "Instance %u: Invalid tune flag [0 - 1, 0 for VQ and 1 for PSNR], your input: %d\n",
            channel_number + 1,
            config->tune);
        return_error = EB_ErrorBadParameter;
    }

    if (config->superres_mode > SUPERRES_AUTO) {
        SVT_ERROR("Instance %u: invalid superres-mode %d, should be in the range [%d - %d]\n",
                  channel_number + 1,
                  config->superres_mode,
                  SUPERRES_NONE,
                  SUPERRES_AUTO);
        return_error = EB_ErrorBadParameter;
    }
    if (config->superres_mode > 0 &&
        ((config->rc_stats_buffer.sz || config->pass == ENC_FIRST_PASS))) {
        SVT_ERROR("Instance %u: superres is not supported for 2-pass\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->superres_qthres > MAX_QP_VALUE) {
        SVT_ERROR("Instance %u: invalid superres-qthres %d, should be in the range [%d - %d] \n",
                  channel_number + 1,
                  config->superres_qthres,
                  MIN_QP_VALUE,
                  MAX_QP_VALUE);
        return_error = EB_ErrorBadParameter;
    }

    if (config->superres_kf_qthres > MAX_QP_VALUE) {
        SVT_ERROR("Instance %u: invalid superres-kf-qthres %d, should be in the range [%d - %d] \n",
                  channel_number + 1,
                  config->superres_kf_qthres,
                  MIN_QP_VALUE,
                  MAX_QP_VALUE);
        return_error = EB_ErrorBadParameter;
    }

    if (config->superres_kf_denom < MIN_SUPERRES_DENOM ||
        config->superres_kf_denom > MAX_SUPERRES_DENOM) {
        SVT_ERROR("Instance %u: invalid superres-kf-denom %d, should be in the range [%d - %d] \n",
                  channel_number + 1,
                  config->superres_kf_denom,
                  MIN_SUPERRES_DENOM,
                  MAX_SUPERRES_DENOM);
        return_error = EB_ErrorBadParameter;
    }

    if (config->superres_denom < MIN_SUPERRES_DENOM ||
        config->superres_denom > MAX_SUPERRES_DENOM) {
        SVT_ERROR("Instance %u: invalid superres-denom %d, should be in the range [%d - %d] \n",
                  channel_number + 1,
                  config->superres_denom,
                  MIN_SUPERRES_DENOM,
                  MAX_SUPERRES_DENOM);
        return_error = EB_ErrorBadParameter;
    }

    if (config->resize_mode > RESIZE_RANDOM_ACCESS) {
        SVT_LOG("Error instance %u: invalid resize-mode %d, should be in the range [%d - %d]\n",
                channel_number + 1,
                config->resize_mode,
                RESIZE_NONE,
                RESIZE_RANDOM_ACCESS);
        return_error = EB_ErrorBadParameter;
    }

    if (config->resize_kf_denom < MIN_RESIZE_DENOM || config->resize_kf_denom > MAX_RESIZE_DENOM) {
        SVT_LOG(
            "Error instance %u: invalid resize-kf-denom %d, should be in the range [%d - %d] \n",
            channel_number + 1,
            config->resize_kf_denom,
            MIN_RESIZE_DENOM,
            MAX_RESIZE_DENOM);
        return_error = EB_ErrorBadParameter;
    }

    if (config->resize_denom < MIN_RESIZE_DENOM || config->resize_denom > MAX_RESIZE_DENOM) {
        SVT_LOG("Error instance %u: invalid resize-denom %d, should be in the range [%d - %d] \n",
                channel_number + 1,
                config->resize_denom,
                MIN_RESIZE_DENOM,
                MAX_RESIZE_DENOM);
        return_error = EB_ErrorBadParameter;
    }

    if (config->matrix_coefficients == 0 && config->encoder_color_format != EB_YUV444) {
        SVT_ERROR(
            "Instance %u: Identity matrix (matrix_coefficient = 0) may be used only with 4:4:4 "
            "color format.\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->hierarchical_levels < 2 || config->hierarchical_levels > 5) {
        SVT_ERROR("Instance %u: Only hierarchical levels 2-5 is currently supported.\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->rate_control_mode == SVT_AV1_RC_MODE_VBR && config->intra_period_length == -1) {
        SVT_ERROR(
            "Instance %u: keyint = -1 is not supported for modes other than CRF rate control "
            "encoding modes.\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    // Limit 8K & 16K configurations ( due to  memory constraints)
    if ((uint64_t)(scs->max_input_luma_width * scs->max_input_luma_height) > INPUT_SIZE_4K_TH &&
        config->enc_mode <= ENC_M7) {
        SVT_ERROR("Instance %u: 8k+ resolution support is limited to M8 and faster presets.\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->enable_adaptive_quantization == 1 &&
        (config->tile_columns > 0 || config->tile_rows > 0)) {
        SVT_ERROR(
            "Instance %u: Adaptive quantization using segmentation is not supported in combination "
            "with tiles.\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->pass > 0 && scs->static_config.enable_overlays) {
        SVT_ERROR(
            "Instance %u: The overlay frames feature is currently not supported with multi-pass "
            "encoding\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    int pass = config->pass;

    if (pass != 3 && pass != 2 && pass != 1 && pass != 0) {
        SVT_ERROR("Instance %u: %d pass encode is not supported. --pass has a range of [0-3]\n",
                  channel_number + 1,
                  pass);
        return_error = EB_ErrorBadParameter;
    }

    if (config->intra_refresh_type != 2 && pass > 0) {
        SVT_ERROR("Instance %u: Multi-pass encode only supports closed-gop configurations.\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (pass == 3 && config->rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF) {
        SVT_ERROR("Instance %u: CRF does not support 3-pass\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->enable_adaptive_quantization == 0 && config->rate_control_mode) {
        SVT_ERROR("Instance %u: Adaptive quantization can not be turned OFF when RC ON\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->sframe_dist < 0) {
        SVT_ERROR("Error instance %u: switch frame interval must be >= 0\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->sframe_dist > 0 && config->pred_structure != SVT_AV1_PRED_LOW_DELAY_P &&
        config->pred_structure != SVT_AV1_PRED_LOW_DELAY_B) {
        SVT_ERROR(
            "Error instance %u: switch frame feature only supports low delay prediction "
            "structure\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->sframe_dist > 0 && config->hierarchical_levels == 0) {
        SVT_ERROR("Error instance %u: switch frame feature does not support flat IPPP\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->sframe_dist > 0 && config->sframe_mode != SFRAME_STRICT_BASE &&
        config->sframe_mode != SFRAME_NEAREST_BASE) {
        SVT_ERROR(
            "Error instance %u: invalid switch frame mode %d, should be in the range [%d - %d]\n",
            channel_number + 1,
            config->sframe_mode,
            SFRAME_STRICT_BASE,
            SFRAME_NEAREST_BASE);
        return_error = EB_ErrorBadParameter;
    }

    /* Warnings about the use of features that are incomplete */
    if (config->enable_adaptive_quantization == 1) {
        config->enable_adaptive_quantization = 2;
        SVT_WARN(
            "Instance %u: Adaptive quantization mode 1 has been disabled, mode 2 will be forced!\n",
            channel_number + 1);
    }

    if (config->pass > 1 && config->rate_control_mode == SVT_AV1_RC_MODE_CQP_OR_CRF) {
        SVT_WARN("Instance %u: CRF does not support Multi-pass. Switching to single pass\n",
                 channel_number + 1);
        config->pass = ENC_SINGLE_PASS;
    }
    // color description
    if (config->color_primaries == 0 || config->color_primaries == 3 ||
        (config->color_primaries >= 13 && config->color_primaries <= 21) ||
        config->color_primaries > 22) {
        SVT_WARN(
            "Instance %u: value %u for color_primaries is reserved and not recommended for "
            "usage.\n",
            channel_number + 1,
            config->color_primaries);
    }
    if (config->transfer_characteristics == 0 || config->transfer_characteristics == 3 ||
        config->transfer_characteristics > 18) {
        SVT_WARN(
            "Instance %u: value %u for transfer_characteristics is reserved and not recommended "
            "for usage.\n",
            channel_number + 1,
            config->transfer_characteristics);
    }

    if (config->matrix_coefficients == 3 || config->matrix_coefficients > 14) {
        SVT_WARN(
            "Instance %u: value %u for matrix_coefficients is reserved and not recommended for "
            "usage.\n",
            channel_number + 1,
            config->matrix_coefficients);
    }

    if (config->chroma_sample_position < EB_CSP_UNKNOWN ||
        config->chroma_sample_position > EB_CSP_COLOCATED) {
        if (config->chroma_sample_position != EB_CSP_RESERVED) {
            SVT_ERROR("Instance %u: chroma sample position %d is unknown.\n",
                      channel_number + 1,
                      config->chroma_sample_position);
            return_error = EB_ErrorBadParameter;
        } else {
            SVT_WARN(
                "Instance %u: value %d for chroma_sample_position is reserved "
                "and not recommended for usage.\n",
                channel_number + 1,
                config->chroma_sample_position);
        }
    }

    if (config->film_grain_denoise_strength > 0 && config->enc_mode > 6) {
        SVT_WARN(
            "Instance %u: It is recommended to not use Film Grain for presets greater than 6 as it "
            "produces a significant compute overhead. This combination should only be used for "
            "debug purposes.\n",
            channel_number + 1);
    }

    if (config->film_grain_denoise_strength > 50) {
        SVT_ERROR(
            "Instance %u: Film grain denoise strength is only supported for values between "
            "[0,50]\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->film_grain_denoise_apply != 0 && config->film_grain_denoise_apply != 1) {
        SVT_ERROR(
            "Instance %u: The film grain denoise apply signal can only have a value of 0 or 1\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    // Limit 8K & 16K support
    if ((uint64_t)(scs->max_input_luma_width * scs->max_input_luma_height) > INPUT_SIZE_4K_TH) {
        SVT_WARN(
            "Instance %u: 8K and higher resolution support is currently a work-in-progress "
            "project, and is only available for demos, experimentation, and further development "
            "uses and should not be used for benchmarking until fully implemented.\n",
            channel_number + 1);
    }

    if (config->pred_structure == 1) {
        SVT_WARN(
            "Instance %u: The low delay encoding mode is a work-in-progress project, and is only "
            "available for demos, experimentation, and further development uses and should not be "
            "used for benchmarking until fully implemented.\n",
            channel_number + 1);
        if (config->tune == 0) {
            SVT_WARN(
                "Instance %u: Tune 0 is not applicable for low-delay, tune will be forced to 1.\n",
                channel_number + 1);
            config->tune = 1;
        }

        if (config->superres_mode != 0) {
            SVT_ERROR("Instance %u: Superres is not supported for low-delay.\n",
                      channel_number + 1);
            return_error = EB_ErrorBadParameter;
        }

        if (config->enable_overlays) {
            SVT_ERROR("Instance %u: Overlay is not supported for low-delay.\n", channel_number + 1);
            return_error = EB_ErrorBadParameter;
        }
    }
    if (scs->static_config.scene_change_detection) {
        scs->static_config.scene_change_detection = 0;
        SVT_WARN(
            "SVT-AV1 has an integrated mode decision mechanism to handle scene changes and will "
            "not insert a key frame at scene changes\n");
    }
    if ((config->tile_columns > 0 || config->tile_rows > 0)) {
        SVT_WARN(
            "If you are using tiles with the intent of increasing the decoder speed, please also "
            "consider using --fast-decode 1, especially if the intended decoder is running with "
            "limited multi-threading capabilities.\n");
    }
    if (config->tune == 0 && config->fast_decode > 0) {
        SVT_WARN(
            "--fast - decode has been developed and optimized with --tune 1. "
            "Please use it with caution when encoding with --tune 0. You can also consider using "
            "--tile-columns 1 if you are targeting a high quality encode and a multi-core "
            "high-performance decoder HW\n");
    }
    if (config->enable_qm && config->min_qm_level > config->max_qm_level) {
        SVT_ERROR(
            "Instance %u:  Min quant matrix level must not greater than max quant matrix level\n",
            channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
#if EN_WARNING_FOR_MISMATCH
    if (config->encoder_bit_depth == 10 &&
        (config->stat_report == 1 || config->recon_enabled == 1) && config->enc_mode >= ENC_M10) {
        SVT_WARN(
            "If encoding 10-bit video using any preset greater than M9 there will be a mismatch "
            "between the encoded files if either --enable-stat-report is 0 or 1 or a recon file "
            "(-o) "
            "is or is not specified in the command line\n");
    }
#endif
#if FTR_STARTUP_MG_SIZE
    if (config->startup_mg_size != 0 && config->startup_mg_size != 2 &&
        config->startup_mg_size != 3 && config->startup_mg_size != 4) {
        SVT_ERROR("Instance %u: Startup MG size supported [0, 2, 3, 4]\n", channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->startup_mg_size >= config->hierarchical_levels) {
        SVT_ERROR("Instance %u: Startup MG size must less than Hierarchical Levels\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->startup_mg_size != 0 && config->rate_control_mode != 0) {
        SVT_ERROR("Instance %u: Startup MG size feature only supports CRF/CQP rate control mode\n",
                  channel_number + 1);
        return_error = EB_ErrorBadParameter;
    }
#endif

    return return_error;
}

// use this function to set the enable_me_8x8 level
static uint8_t get_enable_me_8x8(EncMode enc_mode) {
    uint8_t enable_me_8x8 = 0;
    if (enc_mode <= ENC_M11)
        enable_me_8x8 = 1;
    else
        enable_me_8x8 = 0;

    return enable_me_8x8;
}
#if OPT_LD_M13
static uint8_t get_enable_me_16x16(EncMode enc_mode, bool rtc_tune) {
#else
uint8_t get_enable_me_16x16(EncMode enc_mode) {
#endif
    uint8_t enable_me_16x16;
#if OPT_LD_M13
    if ((enc_mode <= ENC_M12) || (rtc_tune && (enc_mode <= ENC_M13)))
#else
    if (enc_mode <= ENC_M12)
#endif
        enable_me_16x16 = 1;
    else
        enable_me_16x16 = 0;

    return enable_me_16x16;
}

#if OPT_LD_M11
uint8_t svt_aom_get_nic_level(EncMode enc_mode, uint8_t is_base, uint8_t hierarchical_levels,
                              bool rtc_tune);
#else
uint8_t svt_aom_get_nic_level(EncMode enc_mode, uint8_t is_base, uint8_t hierarchical_levels);
#endif
uint8_t svt_aom_get_update_cdf_level(EncMode enc_mode, SliceType is_islice, uint8_t is_base) {
    uint8_t update_cdf_level = 0;
    if (enc_mode <= ENC_M2)
        update_cdf_level = 1;
    else if (enc_mode <= ENC_M5)
        update_cdf_level = is_base ? 1 : 3;
    else if (enc_mode <= ENC_M10)
        update_cdf_level = is_islice ? 1 : 0;
    else
        update_cdf_level = 0;

    return update_cdf_level;
}

uint8_t svt_aom_get_chroma_level(EncMode enc_mode) {
    uint8_t chroma_level = 0;
    if (enc_mode <= ENC_MRS)
        chroma_level = 1;
    else if (enc_mode <= ENC_M2)
        chroma_level = 2;
    else if (enc_mode <= ENC_M6)
        chroma_level = 3;
    else
        chroma_level = 5;

    return chroma_level;
}

EbErrorType signal_derivation_mode_decision_config_kernel_oq(SequenceControlSet *scs,
                                                             PictureControlSet  *pcs) {
    EbErrorType              return_error        = EB_ErrorNone;
    PictureParentControlSet *ppcs                = pcs->ppcs;
    const EncMode            enc_mode            = pcs->enc_mode;
    const uint8_t            is_ref              = ppcs->is_used_as_reference_flag;
    const uint8_t            is_base             = ppcs->temporal_layer_index == 0;
    const uint8_t            is_layer1           = ppcs->temporal_layer_index == 1;
    const EbInputResolution  input_resolution    = ppcs->input_resolution;
    const uint8_t            is_islice           = pcs->slice_type == I_SLICE;
    const SliceType          slice_type          = pcs->slice_type;
    const Bool               fast_decode         = scs->static_config.fast_decode;
    const uint32_t           hierarchical_levels = scs->static_config.hierarchical_levels;
    const Bool               transition_present  = (ppcs->transition_present == 1);
#if OPT_LD_M13
    const bool rtc_tune = (scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) ? true
                                                                                          : false;
#endif
    //MFMV
    if (is_islice || scs->mfmv_enabled == 0 || pcs->ppcs->frm_hdr.error_resilient_mode) {
        ppcs->frm_hdr.use_ref_frame_mvs = 0;
    } else {
        if (fast_decode == 0) {
            if (enc_mode <= ENC_M9)
                ppcs->frm_hdr.use_ref_frame_mvs = 1;
            else {
                uint64_t avg_me_dist = 0;
                for (uint16_t b64_idx = 0; b64_idx < ppcs->b64_total_count; b64_idx++) {
                    avg_me_dist += ppcs->me_64x64_distortion[b64_idx];
                }
                avg_me_dist /= ppcs->b64_total_count;
                avg_me_dist /= pcs->picture_qp;

                ppcs->frm_hdr.use_ref_frame_mvs = avg_me_dist < 200 ||
                        input_resolution <= INPUT_SIZE_360p_RANGE
                    ? 1
                    : 0;
            }
        } else {
            if (enc_mode <= ENC_M9) {
                uint64_t avg_me_dist = 0;
                for (uint16_t b64_idx = 0; b64_idx < ppcs->b64_total_count; b64_idx++) {
                    avg_me_dist += ppcs->me_64x64_distortion[b64_idx];
                }
                avg_me_dist /= ppcs->b64_total_count;
                avg_me_dist /= pcs->picture_qp;

                ppcs->frm_hdr.use_ref_frame_mvs = avg_me_dist < 50 ||
                        input_resolution <= INPUT_SIZE_360p_RANGE
                    ? 1
                    : 0;
            } else {
                ppcs->frm_hdr.use_ref_frame_mvs = input_resolution <= INPUT_SIZE_360p_RANGE ? 1 : 0;
            }
        }
    }

    uint8_t update_cdf_level = svt_aom_get_update_cdf_level(enc_mode, is_islice, is_base);
    //set the conrols uisng the required level
    set_cdf_controls(pcs, update_cdf_level);

    if (pcs->cdf_ctrl.enabled) {
        const uint16_t picture_sb_w = ppcs->picture_sb_width;
        const uint16_t picture_sb_h = ppcs->picture_sb_height;
        const uint16_t all_sb       = picture_sb_w * picture_sb_h;
        rtime_alloc_ec_ctx_array(pcs, all_sb);
    }
    //Filter Intra Mode : 0: OFF  1: ON
    // pic_filter_intra_level specifies whether filter intra would be active
    // for a given picture.

    // pic_filter_intra_level | Settings
    // 0                      | OFF
    // 1                      | ON
    if (scs->filter_intra_level == DEFAULT) {
        if (scs->seq_header.filter_intra_level) {
            if (pcs->enc_mode <= ENC_M4)
                pcs->pic_filter_intra_level = 1;
            else
                pcs->pic_filter_intra_level = 0;
        } else
            pcs->pic_filter_intra_level = 0;
    } else
        pcs->pic_filter_intra_level = scs->filter_intra_level;

    if (fast_decode == 0 || input_resolution <= INPUT_SIZE_360p_RANGE) {
        if (pcs->enc_mode <= ENC_M5)
            pcs->ppcs->partition_contexts = PARTITION_CONTEXTS;
        else
            pcs->ppcs->partition_contexts = 4;
    } else {
        pcs->ppcs->partition_contexts = 4;
    }
    FrameHeader *frm_hdr             = &ppcs->frm_hdr;
    frm_hdr->allow_high_precision_mv = frm_hdr->quantization_params.base_q_idx <
                HIGH_PRECISION_MV_QTHRESH &&
            (scs->input_resolution <= INPUT_SIZE_480p_RANGE)
        ? 1
        : 0;
    // Set Warped Motion level and enabled flag
    pcs->wm_level = 0;
    if (frm_hdr->frame_type == KEY_FRAME || frm_hdr->frame_type == INTRA_ONLY_FRAME ||
        frm_hdr->error_resilient_mode || pcs->ppcs->frame_superres_enabled ||
        pcs->ppcs->frame_resize_enabled) {
        pcs->wm_level = 0;
    } else {
        if (fast_decode == 0 || input_resolution <= INPUT_SIZE_360p_RANGE) {
            if (enc_mode <= ENC_M3) {
                pcs->wm_level = 1;
            } else if (enc_mode <= ENC_M6) {
                if (hierarchical_levels <= 3)
                    pcs->wm_level = is_base ? 1 : 0;
                else
                    pcs->wm_level = (is_base || is_layer1) ? 1 : 0;
            } else if (enc_mode <= ENC_M7) {
                if (hierarchical_levels <= 4)
                    pcs->wm_level = is_base ? 1 : 0;
                else
                    pcs->wm_level = (is_base || is_layer1) ? 1 : 0;
            } else if (enc_mode <= ENC_M10) {
                if (input_resolution <= INPUT_SIZE_720p_RANGE)
                    pcs->wm_level = is_base ? 1 : 0;
                else
                    pcs->wm_level = is_base ? 2 : 0;
            } else {
                pcs->wm_level = is_base ? 2 : 0;
            }
        } else {
            if (enc_mode <= ENC_M7) {
                pcs->wm_level = is_base ? 1 : 0;
            } else if (enc_mode <= ENC_M10) {
                if (input_resolution <= INPUT_SIZE_720p_RANGE)
                    pcs->wm_level = is_base ? 1 : 0;
                else
                    pcs->wm_level = is_base ? 2 : 0;
            } else {
                pcs->wm_level = is_base ? 2 : 0;
            }
        }
    }
    if (hierarchical_levels <= 2) {
        pcs->wm_level = enc_mode <= ENC_M7 ? pcs->wm_level : 0;
    }
    Bool enable_wm = pcs->wm_level ? 1 : 0;
    if (scs->enable_warped_motion != DEFAULT)
        enable_wm = (Bool)scs->enable_warped_motion;
    // Note: local warp should be disabled when super-res or resize is ON
    // according to the AV1 spec 5.11.27
    frm_hdr->allow_warped_motion = enable_wm &&
        !(frm_hdr->frame_type == KEY_FRAME || frm_hdr->frame_type == INTRA_ONLY_FRAME) &&
        !frm_hdr->error_resilient_mode && !pcs->ppcs->frame_superres_enabled &&
        scs->static_config.resize_mode == RESIZE_NONE;

    frm_hdr->is_motion_mode_switchable = frm_hdr->allow_warped_motion;

    // pic_obmc_level - pic_obmc_level is used to define md_pic_obmc_level.
    // The latter determines the OBMC settings in the function set_obmc_controls.
    // Please check the definitions of the flags/variables in the function
    // set_obmc_controls corresponding to the pic_obmc_level settings.
    //  pic_obmc_level  | Default Encoder Settings
    //         0        | OFF subject to possible constraints
    //       > 1        | Faster level subject to possible constraints
    if (scs->obmc_level == DEFAULT) {
        if (fast_decode == 0 || input_resolution <= INPUT_SIZE_360p_RANGE) {
            if (ppcs->enc_mode <= ENC_M2)
                ppcs->pic_obmc_level = 1;
            else if (ppcs->enc_mode <= ENC_M5)
                ppcs->pic_obmc_level = 2;
            else if (enc_mode <= ENC_M6)
                ppcs->pic_obmc_level = 3;
            else
                ppcs->pic_obmc_level = 0;
        } else {
            if (ppcs->enc_mode <= ENC_M3)
                ppcs->pic_obmc_level = 1;
            else if (ppcs->enc_mode <= ENC_M4)
                ppcs->pic_obmc_level = 3;
            else if (ppcs->enc_mode <= ENC_M6)
                ppcs->pic_obmc_level = is_ref ? 3 : 0;
            else
                ppcs->pic_obmc_level = 0;
        }
    } else
        pcs->ppcs->pic_obmc_level = scs->obmc_level;

    // Switchable Motion Mode
    frm_hdr->is_motion_mode_switchable = frm_hdr->is_motion_mode_switchable || ppcs->pic_obmc_level;

    ppcs->bypass_cost_table_gen = 0;
    if (enc_mode <= ENC_M11)
        pcs->approx_inter_rate = 0;
    else
        pcs->approx_inter_rate = 1;
    if (is_islice || transition_present)
        pcs->skip_intra = 0;
#if OPT_LD_M9
    else if ((enc_mode <= ENC_M8) || (rtc_tune && (enc_mode <= ENC_M9)))
#else
    else if (enc_mode <= ENC_M8)
#endif
        pcs->skip_intra = 0;
    else
        pcs->skip_intra = (is_ref || pcs->ref_intra_percentage > 50) ? 0 : 1;

    // Set the level for the candidate(s) reduction feature
    pcs->cand_reduction_level = 0;
    if (is_islice)
        pcs->cand_reduction_level = 0;
    else if (enc_mode <= ENC_M3)
        pcs->cand_reduction_level = 0;
    else if (enc_mode <= ENC_M5)
        pcs->cand_reduction_level = 1;
    else if (enc_mode <= ENC_M6) {
        if (pcs->coeff_lvl == LOW_LVL)
            pcs->cand_reduction_level = 1;
        else
            pcs->cand_reduction_level = 2;
    } else if (enc_mode <= ENC_M9) {
        if (pcs->coeff_lvl == LOW_LVL)
            pcs->cand_reduction_level = 1;
        else
            pcs->cand_reduction_level = 3;
    } else
        pcs->cand_reduction_level = 3;
    if (scs->rc_stat_gen_pass_mode)
        pcs->cand_reduction_level = 7;
    // Set the level for the txt search
    pcs->txt_level = 0;
    if (enc_mode <= ENC_MR)
        pcs->txt_level = 1;
    else if (enc_mode <= ENC_M1)
        pcs->txt_level = 2;
    else if (enc_mode <= ENC_M2)
        pcs->txt_level = 3;
    else if (enc_mode <= ENC_M6) {
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->txt_level = 4;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->txt_level = 7;
        } else { // regular
            pcs->txt_level = is_base ? 4 : 5;
        }
    } else if (enc_mode <= ENC_M11) {
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->txt_level = is_base ? 4 : 5;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->txt_level = is_base ? 8 : 10;
        } else { // regular
            pcs->txt_level = 7;
        }
    }
#if OPT_LD_M13
    else if ((enc_mode <= ENC_M12) || (rtc_tune && (enc_mode <= ENC_M13))) {
#else
    else if (enc_mode <= ENC_M12) {
#endif
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->txt_level = is_base ? 8 : 10;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->txt_level = is_base ? 8 : 10;
            if (pcs->ref_intra_percentage < 85 && !pcs->ppcs->sc_class1) {
                pcs->txt_level = 0;
            }
        } else { // regular
            pcs->txt_level = is_base ? 8 : 10;
            if (pcs->ref_intra_percentage < 85 && !is_base && !pcs->ppcs->sc_class1) {
                pcs->txt_level = 0;
            }
        }
    } else {
        pcs->txt_level = is_base ? 8 : 10;
        if (pcs->ref_intra_percentage < 85 && !pcs->ppcs->sc_class1) {
            pcs->txt_level = 0;
        }
    }
    // Set the level for the txt shortcut feature
    // Any tx_shortcut_level having the chroma detector off in REF frames should be reserved for M13+
    pcs->tx_shortcut_level = 0;
    if (enc_mode <= ENC_M4)
        pcs->tx_shortcut_level = 0;
    else if (enc_mode <= ENC_M5)
        pcs->tx_shortcut_level = is_base ? 0 : 1;
    else if (enc_mode <= ENC_M10)
        pcs->tx_shortcut_level = is_islice ? 0 : 1;
    else
        pcs->tx_shortcut_level = is_islice ? 0 : 4;
    // Set the level the interpolation search
    pcs->interpolation_search_level = 0;

    if (enc_mode <= ENC_MR)
        pcs->interpolation_search_level = 2;
    else if (enc_mode <= ENC_M6)
        pcs->interpolation_search_level = 4;
    else {
        pcs->interpolation_search_level = 4;
        if (!is_base) {
            const uint8_t th[INPUT_SIZE_COUNT] = {100, 100, 85, 50, 30, 30, 30};
            const uint8_t skip_area            = pcs->ref_skip_percentage;
            if (skip_area > th[input_resolution])
                pcs->interpolation_search_level = 0;
        }
    }

    pcs->chroma_level = svt_aom_get_chroma_level(enc_mode);
    // Set the level for cfl
    pcs->cfl_level = 0;
    if (pcs->ppcs->sc_class1) {
        if (enc_mode <= ENC_M6)
            pcs->cfl_level = 1;
        else
            pcs->cfl_level = is_base ? 2 : 0;
    } else if (enc_mode <= ENC_M4)
        pcs->cfl_level = 1;
    else if (enc_mode <= ENC_M9)
        pcs->cfl_level = is_base ? 2 : 0;
    else if (enc_mode <= ENC_M11) {
        if (hierarchical_levels <= 3)
            pcs->cfl_level = is_islice ? 2 : 0;
        else
            pcs->cfl_level = is_base ? 2 : 0;
    } else if (enc_mode <= ENC_M12)
        pcs->cfl_level = is_islice ? 2 : 0;
    else
        pcs->cfl_level = 0;
#if OPT_LD_LATENCY_MD
    if (pcs->scs->low_latency_kf && is_islice)
        pcs->cfl_level = 0;
#endif
    // Set the level for new/nearest/near injection
    if (scs->new_nearest_comb_inject == DEFAULT)
        if (enc_mode <= ENC_M0)
            pcs->new_nearest_near_comb_injection = 1;
        else
            pcs->new_nearest_near_comb_injection = 0;
    else
        pcs->new_nearest_near_comb_injection = scs->new_nearest_comb_inject;

    // Set the level for unipred3x3 injection
    if (enc_mode <= ENC_M0)
        pcs->unipred3x3_injection = 1;
    else
        pcs->unipred3x3_injection = 0;

    // Set the level for bipred3x3 injection
    if (scs->bipred_3x3_inject == DEFAULT) {
        if (enc_mode <= ENC_M0)
            pcs->bipred3x3_injection = 1;
        else if (enc_mode <= ENC_M2)
            pcs->bipred3x3_injection = 2;
        else
            pcs->bipred3x3_injection = 0;
    } else {
        pcs->bipred3x3_injection = scs->bipred_3x3_inject;
    }

    // Set the level for inter-inter compound
    if (scs->compound_mode) {
        if (scs->compound_level == DEFAULT) {
            if (enc_mode <= ENC_MR)
                pcs->inter_compound_mode = 1;
            else if (enc_mode <= ENC_M1)
                pcs->inter_compound_mode = 3;
            else if (enc_mode <= ENC_M3)
                pcs->inter_compound_mode = 4;
            else
                pcs->inter_compound_mode = 0;
        } else {
            pcs->inter_compound_mode = scs->compound_level;
        }
    } else {
        pcs->inter_compound_mode = 0;
    }

    // Set the level for the distance-based red pruning
    if (pcs->ppcs->ref_list0_count_try > 1 || pcs->ppcs->ref_list1_count_try > 1) {
        if (enc_mode <= ENC_MR)
            pcs->dist_based_ref_pruning = 1;
        else if (enc_mode <= ENC_M0)
            pcs->dist_based_ref_pruning = is_base ? 1 : 2;
        else if (enc_mode <= ENC_M4)
            pcs->dist_based_ref_pruning = is_base ? 2 : 5;
        else if (enc_mode <= ENC_M5)
            pcs->dist_based_ref_pruning = is_base ? 2 : 6;
        else {
            if (pcs->coeff_lvl == LOW_LVL) {
                pcs->dist_based_ref_pruning = is_base ? 2 : 6;
            } else {
                pcs->dist_based_ref_pruning = is_base ? 3 : 6;
            }
        }
    } else {
        pcs->dist_based_ref_pruning = 0;
    }

    // Set the level the spatial sse @ full-loop
    pcs->spatial_sse_full_loop_level = 0;
    if (scs->spatial_sse_full_loop_level == DEFAULT)
        if (pcs->ppcs->sc_class1)
            pcs->spatial_sse_full_loop_level = 1;
        else if (enc_mode <= ENC_M11)
            pcs->spatial_sse_full_loop_level = 1;
        else
            pcs->spatial_sse_full_loop_level = 0;
    else
        pcs->spatial_sse_full_loop_level = scs->spatial_sse_full_loop_level;
    //set the nsq_level
    pcs->nsq_level = svt_aom_get_nsq_level(enc_mode, is_islice, is_base, pcs->coeff_lvl);
    // Set the level for enable_inter_intra
    // Block level switch, has to follow the picture level
    // inter intra pred                      Settings
    // 0                                     OFF
    // 1                                     FULL
    // 2                                     FAST 1 : Do not inject for unipred3x3 or PME inter candidates
    // 3                                     FAST 2 : Level 1 + do not inject for non-closest ref frames or ref frames with high distortion
    if (pcs->ppcs->slice_type != I_SLICE && scs->seq_header.enable_interintra_compound) {
        if (enc_mode <= ENC_M1)
            pcs->md_inter_intra_level = 1;
        else if (enc_mode <= ENC_M2)
            pcs->md_inter_intra_level = transition_present ? 1 : (is_base ? 1 : 0);
        else if (enc_mode <= ENC_M11)
            pcs->md_inter_intra_level = transition_present ? 1 : 0;
        else
            pcs->md_inter_intra_level = 0;
    } else
        pcs->md_inter_intra_level = 0;

    if (enc_mode <= ENC_MRS)
        pcs->txs_level = 1;
    else if (enc_mode <= ENC_MR)
        pcs->txs_level = 2;
    else if (enc_mode <= ENC_M2)
        pcs->txs_level = is_base ? 2 : 3;
    else if (enc_mode <= ENC_M7)
        pcs->txs_level = is_base ? 2 : 0;
#if OPT_LD_M10
    else if ((enc_mode <= ENC_M9) || (rtc_tune && enc_mode <= ENC_M10))
#else
    else if (enc_mode <= ENC_M9)
#endif
        pcs->txs_level = is_islice ? 3 : 0;
    else if (enc_mode <= ENC_M10) {
        if (hierarchical_levels <= 3)
            pcs->txs_level = is_islice ? 5 : 0;
        else
            pcs->txs_level = is_islice ? 3 : 0;
    } else
        pcs->txs_level = is_islice ? 5 : 0;
#if OPT_LD_LATENCY_MD
    if (pcs->scs->low_latency_kf && pcs->slice_type == I_SLICE)
        pcs->txs_level = 5;
#endif
    // Set tx_mode for the frame header
    frm_hdr->tx_mode = (pcs->txs_level) ? TX_MODE_SELECT : TX_MODE_LARGEST;
    // Set the level for nic
#if OPT_LD_M11
    pcs->nic_level = svt_aom_get_nic_level(enc_mode, is_base, hierarchical_levels, rtc_tune);
#else
    pcs->nic_level = svt_aom_get_nic_level(enc_mode, is_base, hierarchical_levels);
#endif
    // Set the level for SQ me-search
    if (enc_mode <= ENC_M0)
        pcs->md_sq_mv_search_level = 1;
    else
        pcs->md_sq_mv_search_level = 0;

    // Set the level for NSQ me-search
    if (enc_mode <= ENC_MRS)
        pcs->md_nsq_mv_search_level = 2;
    else
        pcs->md_nsq_mv_search_level = 4;
    // Set the level for PME search
    if (enc_mode <= ENC_MR)
        pcs->md_pme_level = 1;
    else if (enc_mode <= ENC_M3)
        pcs->md_pme_level = 2;
    else if (enc_mode <= ENC_M5) {
        if (hierarchical_levels <= 3)
            pcs->md_pme_level = 6;
        else
            pcs->md_pme_level = 3;
    } else if (enc_mode <= ENC_M7)
        pcs->md_pme_level = 5;
    else
        pcs->md_pme_level = 6;

    // Set the level for mds0
    pcs->mds0_level = 0;
#if OPT_LD_M10
    if ((enc_mode <= ENC_M9) || (rtc_tune && enc_mode <= ENC_M10))
#else
    if (enc_mode <= ENC_M9)
#endif
        pcs->mds0_level = 2;
    else if (enc_mode <= ENC_M11) {
        if (hierarchical_levels <= 3)
            pcs->mds0_level = is_islice ? 2 : 4;
        else
            pcs->mds0_level = 2;
    } else
        pcs->mds0_level = is_islice ? 2 : 4;
    /*
       disallow_4x4
    */
    pcs->pic_disallow_4x4 = svt_aom_get_disallow_4x4(enc_mode, slice_type);
    /*
       Bypassing EncDec
    */
#if OPT_LD_P2
    // In low delay mode, bypassing encdec in base layer is disabled as it provides bad trade offs
#endif
    // TODO: Bypassing EncDec doesn't work if NSQ is enabled for 10bit content (causes r2r).
    // TODO: This signal can only be modified per picture right now, not per SB.  Per SB requires
    // neighbour array updates at EncDec for all SBs, that are currently skipped if EncDec is bypassed.
    // TODO: Bypassing EncDec doesn't work if pcs->cdf_ctrl.update_coef is enabled for non-ISLICE frames (causes r2r)
#if OPT_LD_P2
    if (!(is_base && scs->static_config.pred_structure == SVT_AV1_PRED_LOW_DELAY_B) &&
        (scs->static_config.encoder_bit_depth == EB_EIGHT_BIT || !pcs->nsq_level) &&
#else
    if ((scs->static_config.encoder_bit_depth == EB_EIGHT_BIT || !pcs->nsq_level) &&
#endif
        (!pcs->cdf_ctrl.update_coef || is_islice) &&
        !ppcs->frm_hdr.segmentation_params.segmentation_enabled) {
        pcs->pic_bypass_encdec = svt_aom_get_bypass_encdec(
            enc_mode, ppcs->hbd_md, scs->static_config.encoder_bit_depth);
    } else
        pcs->pic_bypass_encdec = 0;

    /*
        set lpd0_level
    */
    if (enc_mode <= ENC_M2)
        pcs->pic_lpd0_lvl = 0;
    else if (enc_mode <= ENC_M5)
        pcs->pic_lpd0_lvl = 1;
    else if (enc_mode <= ENC_M9) {
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->pic_lpd0_lvl = 1;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->pic_lpd0_lvl = (is_base || transition_present) ? 2 : 4;
        } else { // Regular
            pcs->pic_lpd0_lvl = 2;
        }
    } else if (enc_mode <= ENC_M10) {
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->pic_lpd0_lvl = 2;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 6;
        } else { // Regular
            pcs->pic_lpd0_lvl = (is_base || transition_present) ? 2 : 4;
        }
    } else if (enc_mode <= ENC_M11) {
#if OPT_LD_M11
        if (rtc_tune) {
            if (pcs->coeff_lvl == LOW_LVL) {
                pcs->pic_lpd0_lvl = 2;
            } else if (pcs->coeff_lvl == HIGH_LVL) {
                pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 7;
            } else { // Regular
                pcs->pic_lpd0_lvl = (is_base || transition_present) ? 4 : 6;
            }
        } else {
            if (pcs->coeff_lvl == LOW_LVL) {
                pcs->pic_lpd0_lvl = (is_base || transition_present) ? 2 : 4;
            } else if (pcs->coeff_lvl == HIGH_LVL) {
                pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 7;
            } else { // Regular
                pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 6;
            }
        }
#else
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->pic_lpd0_lvl = (is_base || transition_present) ? 2 : 4;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 7;
        } else { // Regular
            pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 6;
        }
#endif
    } else {
#if OPT_LD_P2
        if (rtc_tune) {
            if (enc_mode <= ENC_M12) {
                if (pcs->coeff_lvl == LOW_LVL) {
                    pcs->pic_lpd0_lvl = 4;
                } else if (pcs->coeff_lvl == HIGH_LVL) {
                    pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 7;
                } else { // Regular
                    pcs->pic_lpd0_lvl = (is_base || transition_present) ? 4 : 6;
                }
            } else
                pcs->pic_lpd0_lvl = is_islice ? 4 : 7;
        } else {
#endif
            if (pcs->coeff_lvl == LOW_LVL) {
                pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 6;
            } else if (pcs->coeff_lvl == HIGH_LVL) {
                pcs->pic_lpd0_lvl = 7;
            } else { // regular
                pcs->pic_lpd0_lvl = (is_base || transition_present) ? 5 : 7;
            }
#if OPT_LD_P2
        }
#endif
    }

    if (pcs->ppcs->sc_class1 || scs->static_config.pass == ENC_MIDDLE_PASS)
        pcs->pic_skip_pd0 = 0;
    else if (enc_mode <= ENC_M13)
        pcs->pic_skip_pd0 = 0;
    else
        pcs->pic_skip_pd0 = is_base ? 0 : 1;
#if OPT_LD_M9
    pcs->pic_disallow_below_16x16 = svt_aom_get_disallow_below_16x16_picture_level(
        enc_mode, input_resolution, is_islice, ppcs->sc_class1, is_ref, rtc_tune);
#else
    pcs->pic_disallow_below_16x16 = svt_aom_get_disallow_below_16x16_picture_level(
        enc_mode, input_resolution, is_islice, ppcs->sc_class1, is_ref);
#endif

    if (scs->super_block_size == 64) {
        if (is_islice || transition_present) {
            pcs->pic_depth_removal_level = 0;
        } else {
            // Set depth_removal_level_controls
            if (pcs->ppcs->sc_class1) {
                if (enc_mode <= ENC_M8)
                    pcs->pic_depth_removal_level = 0;
                else if (enc_mode <= ENC_M10) {
                    pcs->pic_depth_removal_level = is_base ? 0 : 6;
                } else if (enc_mode <= ENC_M12) {
                    pcs->pic_depth_removal_level = is_base ? 4 : 6;
                } else {
                    pcs->pic_depth_removal_level = is_base ? 5 : 14;
                }
            } else if (fast_decode == 0) {
                if (enc_mode <= ENC_M1)
                    pcs->pic_depth_removal_level = 0;
                else if (enc_mode <= ENC_M5) {
                    if (input_resolution <= INPUT_SIZE_480p_RANGE)
                        pcs->pic_depth_removal_level = 1;
                    else
                        pcs->pic_depth_removal_level = 2;
                } else if (enc_mode <= ENC_M7) {
                    if (pcs->coeff_lvl == LOW_LVL) {
                        if (input_resolution <= INPUT_SIZE_480p_RANGE)
                            pcs->pic_depth_removal_level = 1;
                        else
                            pcs->pic_depth_removal_level = 2;
                    } else {
                        if (input_resolution <= INPUT_SIZE_480p_RANGE)
                            pcs->pic_depth_removal_level = 1;
                        else
                            pcs->pic_depth_removal_level = 2;
                    }
                } else if (enc_mode <= ENC_M8) {
                    if (pcs->coeff_lvl == LOW_LVL) {
                        if (input_resolution <= INPUT_SIZE_480p_RANGE)
                            pcs->pic_depth_removal_level = 1;
                        else
                            pcs->pic_depth_removal_level = 2;
                    } else {
                        if (input_resolution <= INPUT_SIZE_480p_RANGE)
                            pcs->pic_depth_removal_level = 1;
                        else
                            pcs->pic_depth_removal_level = is_base ? 2 : 6;
                    }
                } else if (enc_mode <= ENC_M9) {
                    if (pcs->coeff_lvl == LOW_LVL) {
                        if (input_resolution <= INPUT_SIZE_480p_RANGE)
                            pcs->pic_depth_removal_level = 1;
                        else
                            pcs->pic_depth_removal_level = 2;
                    } else {
                        if (input_resolution <= INPUT_SIZE_360p_RANGE)
                            pcs->pic_depth_removal_level = is_base ? 2 : 3;
                        else if (input_resolution <= INPUT_SIZE_480p_RANGE)
                            pcs->pic_depth_removal_level = is_base ? 2 : 5;
                        else
                            pcs->pic_depth_removal_level = is_base ? 2 : 6;
                    }
                } else if (enc_mode <= ENC_M11) {
                    if (input_resolution <= INPUT_SIZE_360p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 3;
                    else if (input_resolution <= INPUT_SIZE_480p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 5;
                    else if (input_resolution <= INPUT_SIZE_720p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 6;
                    else if (input_resolution <= INPUT_SIZE_1080p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 3 : 8;
                    else
                        pcs->pic_depth_removal_level = is_base ? 9 : 14;
                } else {
                    if (input_resolution <= INPUT_SIZE_360p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 3;
                    else if (input_resolution <= INPUT_SIZE_480p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 9 : 11;
                    else
                        pcs->pic_depth_removal_level = is_base ? 9 : 14;
                }
            } else {
                if (enc_mode <= ENC_M2)
                    pcs->pic_depth_removal_level = 0;
                else if (enc_mode <= ENC_M5) {
                    if (input_resolution <= INPUT_SIZE_480p_RANGE)
                        pcs->pic_depth_removal_level = 1;
                    else
                        pcs->pic_depth_removal_level = 2;
                } else if (enc_mode <= ENC_M7) {
                    if (input_resolution <= INPUT_SIZE_1080p_RANGE)
                        pcs->pic_depth_removal_level = 1;
                    else
                        pcs->pic_depth_removal_level = 2;
                } else if (enc_mode <= ENC_M8) {
                    if (input_resolution <= INPUT_SIZE_360p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 3;
                    else if (input_resolution <= INPUT_SIZE_480p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 5;
                    else
                        pcs->pic_depth_removal_level = is_base ? 2 : 6;
                } else if (enc_mode <= ENC_M11) {
                    if (input_resolution <= INPUT_SIZE_360p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 4;
                    else if (input_resolution <= INPUT_SIZE_480p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 5;
                    else if (input_resolution <= INPUT_SIZE_720p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 2 : 6;
                    else if (input_resolution <= INPUT_SIZE_1080p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 3 : 8;
                    else
                        pcs->pic_depth_removal_level = is_base ? 9 : 14;
                } else {
                    if (input_resolution <= INPUT_SIZE_360p_RANGE)
                        pcs->pic_depth_removal_level = 7;
                    else if (input_resolution <= INPUT_SIZE_480p_RANGE)
                        pcs->pic_depth_removal_level = is_base ? 9 : 11;
                    else
                        pcs->pic_depth_removal_level = is_base ? 9 : 14;
                }
            }
        }
    }
    if (pcs->ppcs->sc_class1) {
        if (enc_mode <= ENC_M6)
            pcs->pic_block_based_depth_refinement_level = 0;
        else if (enc_mode <= ENC_M9)
            pcs->pic_block_based_depth_refinement_level = is_base ? 0 : 5;
        else if (enc_mode <= ENC_M10)
            pcs->pic_block_based_depth_refinement_level = is_islice ? 1 : 5;
        else
            pcs->pic_block_based_depth_refinement_level = is_islice ? 7 : 12;
    } else if (enc_mode <= ENC_M2)
        pcs->pic_block_based_depth_refinement_level = 0;
    else if (enc_mode <= ENC_M5) {
        if (pcs->coeff_lvl == LOW_LVL)
            pcs->pic_block_based_depth_refinement_level = is_base ? 0 : 2;
        else
            pcs->pic_block_based_depth_refinement_level = is_base ? 0 : 4;
    } else {
#if OPT_LD_M9
        if (rtc_tune) {
            if (enc_mode <= ENC_M8) {
                if (pcs->coeff_lvl == LOW_LVL) {
                    pcs->pic_block_based_depth_refinement_level = is_base ? 1 : 4;
                } else { // regular
                    pcs->pic_block_based_depth_refinement_level = is_base ? 2 : 5;
                }
            } else {
                if (pcs->coeff_lvl == LOW_LVL) {
                    pcs->pic_block_based_depth_refinement_level = is_base ? 2 : 6;
                } else { // regular
                    pcs->pic_block_based_depth_refinement_level = is_base ? 7 : 10;
                }
            }
        } else {
            if (pcs->coeff_lvl == LOW_LVL) {
                pcs->pic_block_based_depth_refinement_level = is_base ? 1 : 4;
            } else { // regular
                pcs->pic_block_based_depth_refinement_level = is_base ? 2 : 5;
            }
        }
#else
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->pic_block_based_depth_refinement_level = is_base ? 1 : 4;
        } else { // regular
            pcs->pic_block_based_depth_refinement_level = is_base ? 2 : 5;
        }
#endif
    }
#if FIX_LAYER_SIGNAL
    if (ppcs->hierarchical_levels == (EB_MAX_TEMPORAL_LAYERS - 1)) {
        pcs->pic_block_based_depth_refinement_level = MAX(
            0, pcs->pic_block_based_depth_refinement_level - 1);
    }
#else
    if (scs->max_heirachical_level == (EB_MAX_TEMPORAL_LAYERS - 1)) {
        pcs->pic_block_based_depth_refinement_level = MAX(
            0, pcs->pic_block_based_depth_refinement_level - 1);
    }
#endif
    if (enc_mode <= ENC_M1)
        pcs->pic_depth_early_exit_th = 0;
    else
        pcs->pic_depth_early_exit_th = 90;

    if (pcs->ppcs->sc_class1) {
        if (enc_mode <= ENC_M7)
            pcs->pic_lpd1_lvl = 0;
        else if (enc_mode <= ENC_M9)
            pcs->pic_lpd1_lvl = is_ref ? 0 : 1;
        else if (enc_mode <= ENC_M10)
            pcs->pic_lpd1_lvl = is_ref ? 0 : 2;
        else if (enc_mode <= ENC_M11)
            pcs->pic_lpd1_lvl = is_base ? 0 : 2;
        else
            pcs->pic_lpd1_lvl = is_base ? 0 : 4;
    } else if (enc_mode <= ENC_M7)
        pcs->pic_lpd1_lvl = 0;
    else if (enc_mode <= ENC_M9) {
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->pic_lpd1_lvl = 0;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->pic_lpd1_lvl = is_base ? 0 : 2;
        } else { // Regular
            pcs->pic_lpd1_lvl = is_ref ? 0 : 1;
        }
    } else if (enc_mode <= ENC_M10) {
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->pic_lpd1_lvl = is_ref ? 0 : 1;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->pic_lpd1_lvl = is_base ? 0 : 3;
        } else { // Regular
            pcs->pic_lpd1_lvl = is_base ? 0 : 2;
        }
#if OPT_LD_M12_13
    } else if ((enc_mode <= ENC_M11) || (rtc_tune && (enc_mode <= ENC_M12))) {
#else
    } else if (enc_mode <= ENC_M11) {
#endif
#if OPT_LD_M11
        if (rtc_tune) {
            if (pcs->coeff_lvl == LOW_LVL) {
                pcs->pic_lpd1_lvl = is_ref ? 0 : 2;
            } else if (pcs->coeff_lvl == HIGH_LVL) {
                pcs->pic_lpd1_lvl = is_base ? 0 : 4;
            } else { // Regular
                pcs->pic_lpd1_lvl = is_base ? 0 : 3;
            }
        } else {
            if (pcs->coeff_lvl == LOW_LVL) {
                pcs->pic_lpd1_lvl = is_base ? 0 : 2;
            } else if (pcs->coeff_lvl == HIGH_LVL) {
                pcs->pic_lpd1_lvl = is_base ? 0 : 4;
            } else { // Regular
                pcs->pic_lpd1_lvl = is_base ? 0 : 3;
            }
        }
#else
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->pic_lpd1_lvl = is_base ? 0 : 2;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->pic_lpd1_lvl = is_base ? 0 : 4;
        } else { // Regular
            pcs->pic_lpd1_lvl = is_base ? 0 : 3;
        }
#endif
#if OPT_LD_M13
    } else if ((enc_mode <= ENC_M12) || (rtc_tune && (enc_mode <= ENC_M13))) {
#else
    } else if (enc_mode <= ENC_M12) {
#endif
        if (pcs->coeff_lvl == LOW_LVL) {
            pcs->pic_lpd1_lvl = is_base ? 0 : 3;
        } else if (pcs->coeff_lvl == HIGH_LVL) {
            pcs->pic_lpd1_lvl = is_base ? 0 : 5;
        } else { // regular
            pcs->pic_lpd1_lvl = is_base ? 0 : 4;
        }
    } else {
        if (input_resolution <= INPUT_SIZE_1080p_RANGE &&
            scs->static_config.encoder_bit_depth == EB_EIGHT_BIT)
            pcs->pic_lpd1_lvl = is_base ? 0 : 6;
        else
            pcs->pic_lpd1_lvl = is_base ? 0 : 5;
    }
    // Can only use light-PD1 under the following conditions
    // There is another check before PD1 is called; pred_depth_only is not checked here, because some modes
    // may force pred_depth_only at the light-pd1 detector
    if (pcs->pic_lpd1_lvl &&
        !(ppcs->hbd_md == 0 && !pcs->nsq_level && pcs->pic_disallow_4x4 == TRUE &&
          scs->super_block_size == 64)) {
        pcs->pic_lpd1_lvl = 0;
    }

    // Use the me-SAD-to-SATD deviation (of the 32x32 blocks) to detect the presence of isolated edges.
    // An SB is tagged as problematic when the deviation is higher than the normal (i.e. when me-sad and satd are not correlated)
    // For the detected SB(s), apply a better level for Depth-removal, LPD0, LPD1, and TXT of regular PD1.
    // Not applicable for I_SLICE and for SB 128x128
    if (pcs->slice_type == I_SLICE || scs->super_block_size == 128) {
        pcs->vq_ctrls.detect_high_freq_lvl = 0;
    } else {
        if (pcs->ppcs->sc_class1) {
            pcs->vq_ctrls.detect_high_freq_lvl = 1;
        } else {
#if OPT_LD_M13
            if ((enc_mode <= ENC_M12) || (rtc_tune && (enc_mode <= ENC_M13)))
#else
            if (enc_mode <= ENC_M12)
#endif
                pcs->vq_ctrls.detect_high_freq_lvl = 2;
            else
                pcs->vq_ctrls.detect_high_freq_lvl = 0;
        }
    }

    return return_error;
}