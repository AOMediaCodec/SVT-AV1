#include <stdio.h>
#include <stdlib.h>
#include "EbUtility.h"
#include "EbInterPrediction.h"
#include "EbResize.h"
#include "EbIntraPrediction.h"
#include "EbDefinitions.h"
#include "EbRateControlProcess.h"
#include "EbModeDecisionConfigurationProcess.h"
#include "EbEncDecProcess.h"
#include "EbMotionEstimationProcess.h"
#include "EbPictureDecisionProcess.h"
#include "firstpass.h"
#include "EbResourceCoordinationProcess.h"

EbErrorType       first_pass_signal_derivation_mode_decision_config_kernel(PictureControlSet *pcs);
uint8_t svt_aom_get_bypass_encdec(EncMode enc_mode, uint8_t hbd_md, uint8_t encoder_bit_depth);

uint8_t     svt_aom_get_bypass_encdec(EncMode enc_mode, uint8_t hbd_md, uint8_t encoder_bit_depth);
#if OPT_LD_M11
uint8_t svt_aom_get_nic_level(EncMode enc_mode, uint8_t is_base, uint8_t hierarchical_levels,
                              bool rtc_tune);
#else
uint8_t svt_aom_get_nic_level(EncMode enc_mode, uint8_t is_base, uint8_t hierarchical_levels);
#endif
uint8_t svt_aom_get_update_cdf_level(EncMode enc_mode, SliceType is_islice, uint8_t is_base);
uint8_t svt_aom_get_chroma_level(EncMode enc_mode);
void svt_aom_sb_qp_derivation_tpl_la(PictureControlSet *pcs);
void svt_aom_md_pme_search_controls(ModeDecisionContext *ctx, uint8_t md_pme_level);
void svt_aom_set_inter_intra_ctrls(ModeDecisionContext *ctx, uint8_t inter_intra_level);
void svt_aom_set_txt_controls(ModeDecisionContext *mdctxt, uint8_t txt_level);
uint8_t svt_aom_set_nic_controls(ModeDecisionContext *ctx, uint8_t nic_level);
uint8_t svt_aom_set_chroma_controls(ModeDecisionContext *ctx, uint8_t uv_level);
MvReferenceFrame svt_get_ref_frame_type(uint8_t list, uint8_t ref_idx);
extern MvReferenceFrame svt_get_ref_frame_type(uint8_t list, uint8_t ref_idx);
void svt_aom_set_me_hme_ref_prune_ctrls(MeContext *me_ctx, uint8_t prune_level);
void svt_aom_set_me_sr_adjustment_ctrls(MeContext *me_ctx, uint8_t sr_adjustment_level);
void svt_aom_set_prehme_ctrls(MeContext *me_ctx, uint8_t level);
uint16_t  svt_aom_get_max_can_count(EncMode enc_mode );
void svt_aom_set_wn_filter_ctrls(Av1Common *cm, uint8_t wn_filter_lvl);
void svt_aom_set_sg_filter_ctrls(Av1Common *cm, uint8_t wn_filter_lvl);
uint8_t svt_aom_get_wn_filter_level(EncMode enc_mode, uint8_t input_resolution, Bool is_ref);
uint8_t svt_aom_get_tpl_synthesizer_block_size(int8_t tpl_level, uint32_t picture_width, uint32_t picture_height);
void set_skip_frame_in_ipp(PictureParentControlSet *  pcs, MeContext *me_ctx);

extern EbErrorType first_pass_signal_derivation_pre_analysis_pcs(PictureParentControlSet *pcs);
extern EbErrorType first_pass_signal_derivation_pre_analysis_scs(SequenceControlSet *scs);
EbErrorType signal_derivation_block(PictureControlSet *pcs, ModeDecisionContext *ctx);
EbErrorType signal_derivation_mode_decision_config_kernel_oq(SequenceControlSet *scs,
                                                             PictureControlSet  *pcs);
EbPictureBufferDesc *svt_aom_get_ref_pic_buffer(PictureControlSet *pcs, uint8_t is_highbd,
                                                uint8_t list_idx, uint8_t ref_idx);
EbErrorType signal_derivation_pre_analysis_oq_pcs(PictureParentControlSet *pcs);
EbErrorType first_pass_signal_derivation_multi_processes(SequenceControlSet *     scs,
                                                         PictureParentControlSet *pcs);
EbErrorType first_pass_signal_derivation_me_kernel(SequenceControlSet *       scs,
                                                   PictureParentControlSet *  pcs,
                                                   MotionEstimationContext_t *me_context_ptr);
EbErrorType signal_derivation_multi_processes_oq(
    SequenceControlSet *scs,
    PictureParentControlSet *pcs,
    PictureDecisionContext *context_ptr);
    
EbErrorType signal_derivation_enc_dec_kernel_common(SequenceControlSet  *scs,
                                                           PictureControlSet   *pcs,
                                                           ModeDecisionContext *ctx);
                                                           
void signal_derivation_enc_dec_kernel_oq_light_pd0(SequenceControlSet  *scs,
                                                          PictureControlSet   *pcs,
                                                          ModeDecisionContext *ctx);
void set_depth_ctrls(ModeDecisionContext *ctx, uint8_t depth_level);
Bool is_ref_same_size(PictureControlSet *pcs, uint8_t list_idx, uint8_t ref_idx);

EbErrorType tf_signal_derivation_me_kernel_oq(PictureParentControlSet *  pcs,
                                              MotionEstimationContext_t *me_context_ptr);
void set_tpl_extended_controls(PictureParentControlSet *pcs, uint8_t tpl_level);

void svt_aom_residual_kernel(uint8_t *input, uint32_t input_offset, uint32_t input_stride,
                             uint8_t *pred, uint32_t pred_offset, uint32_t pred_stride,
                             int16_t *residual, uint32_t residual_offset, uint32_t residual_stride,
                             Bool hbd, uint32_t area_width, uint32_t area_height);
EbErrorType signal_derivation_pre_analysis_oq_scs(SequenceControlSet *scs);                             
                             
void signal_derivation_enc_dec_kernel_oq_light_pd1(PictureControlSet   *pcs,
                                                   ModeDecisionContext *ctx);       
                                