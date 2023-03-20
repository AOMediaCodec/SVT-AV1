#include <stdio.h>
#include <stdlib.h>
#include "EbUtility.h"
#include "EbInterPrediction.h"
#include "EbResize.h"
#include "EbIntraPrediction.h"
#include "EbDefinitions.h"
#include "EbMotionEstimationProcess.h"
#include "EbModeDecisionConfigurationProcess.h"
#include "EbResourceCoordinationProcess.h"
#include "EbEncDecProcess.h"
#include "firstpass.h"
#include "EbPictureDecisionProcess.h"
#include "EbPictureBufferDesc.h"
#include "EbEncInterPrediction.h"

void svt_aom_set_me_hme_ref_prune_ctrls(MeContext *me_ctx, uint8_t prune_level);
void svt_aom_set_me_sr_adjustment_ctrls(MeContext *me_ctx, uint8_t sr_adjustment_level);
void svt_aom_set_prehme_ctrls(MeContext *me_ctx, uint8_t level);
uint16_t  svt_aom_get_max_can_count(EncMode enc_mode );
void svt_aom_set_wn_filter_ctrls(Av1Common *cm, uint8_t wn_filter_lvl);
void svt_aom_set_sg_filter_ctrls(Av1Common *cm, uint8_t wn_filter_lvl);
uint8_t svt_aom_get_wn_filter_level(EncMode enc_mode, uint8_t input_resolution, Bool is_ref);
void set_skip_frame_in_ipp(PictureParentControlSet *  pcs, MeContext *me_ctx);
void svt_aom_md_pme_search_controls(ModeDecisionContext *ctx, uint8_t md_pme_level);
void svt_aom_set_inter_intra_ctrls(ModeDecisionContext *ctx, uint8_t inter_intra_level);
void svt_aom_set_txt_controls(ModeDecisionContext *mdctxt, uint8_t txt_level);
uint8_t svt_aom_set_nic_controls(ModeDecisionContext *ctx, uint8_t nic_level);
uint8_t svt_aom_set_chroma_controls(ModeDecisionContext *ctx, uint8_t uv_level);
uint8_t     svt_aom_get_bypass_encdec(EncMode enc_mode, uint8_t hbd_md, uint8_t encoder_bit_depth);
uint8_t svt_aom_get_update_cdf_level(EncMode enc_mode, SliceType is_islice, uint8_t is_base);
uint8_t svt_aom_get_chroma_level(EncMode enc_mode);
uint8_t svt_aom_get_bypass_encdec(EncMode enc_mode, uint8_t hbd_md, uint8_t encoder_bit_depth);
#if OPT_LD_M11
uint8_t svt_aom_get_nic_level(EncMode enc_mode, uint8_t is_base, uint8_t hierarchical_levels,
                              bool rtc_tune);
#else
uint8_t svt_aom_get_nic_level(EncMode enc_mode, uint8_t is_base, uint8_t hierarchical_levels);
#endif

// uint8_t svt_aom_get_disallow_below_16x16_picture_level(EncMode           enc_mode,
                                                       // EbInputResolution resolution, Bool is_islice,
                                                       // Bool sc_class1, Bool is_ref, Bool rtc_tune);

void set_depth_ctrls(ModeDecisionContext *ctx, uint8_t depth_level);

#if OPT_LD_M13
uint8_t get_enable_me_16x16(EncMode enc_mode, bool rtc_tune);
#else
uint8_t get_enable_me_16x16(EncMode enc_mode);
#endif

Bool is_ref_same_size(PictureControlSet *pcs, uint8_t list_idx, uint8_t ref_idx);
uint8_t get_enable_me_8x8(EncMode enc_mode);
void set_tpl_extended_controls(PictureParentControlSet *pcs, uint8_t tpl_level);


uint8_t get_enable_me_8x8(EncMode enc_mode);
 
                                       

void sig_deriv_mode_decision_config(SequenceControlSet *scs,
                                                             PictureControlSet  *pcs);
                                                             
void first_pass_sig_deriv_mode_decision_config(PictureControlSet *pcs);

void sig_deriv_block(PictureControlSet *pcs, ModeDecisionContext *ctx);
void sig_deriv_pre_analysis_pcs(PictureParentControlSet *pcs);

extern void first_pass_sig_deriv_pre_analysis_pcs(PictureParentControlSet *pcs);

void sig_deriv_pre_analysis_scs(SequenceControlSet *scs);
                                                            

extern void first_pass_sig_deriv_pre_analysis_scs(SequenceControlSet *scs);
void sig_deriv_multi_processes(
    SequenceControlSet *scs,
    PictureParentControlSet *pcs,
    PictureDecisionContext *context_ptr);
    
void first_pass_sig_deriv_multi_processes(SequenceControlSet *     scs,
                                                         PictureParentControlSet *pcs);
                                                         
void sig_deriv_me_tf(PictureParentControlSet *  pcs,
                                              MotionEstimationContext_t *me_context_ptr);

void first_pass_sig_deriv_me(SequenceControlSet *       scs,
                                                   PictureParentControlSet *  pcs,
                                                   MotionEstimationContext_t *me_context_ptr);


void sig_deriv_enc_dec_light_pd1(PictureControlSet   *pcs,
                                                   ModeDecisionContext *ctx);
void sig_deriv_enc_dec_light_pd0(SequenceControlSet  *scs,
                                                          PictureControlSet   *pcs,
                                                          ModeDecisionContext *ctx);
void sig_deriv_enc_dec__common(SequenceControlSet  *scs,
                                                           PictureControlSet   *pcs,
                                                           ModeDecisionContext *ctx);                                                          