# Scalable Video Technology for AV1 (SVT-AV1 Encoder and Decoder)

The Scalable Video Technology for AV1 (SVT-AV1 Encoder and Decoder) is an AV1-compliant encoder/decoder library core. The SVT-AV1 encoder development is a work-in-progress targeting performance levels applicable to both VOD and Live encoding / transcoding video applications. The SVT-AV1 decoder implementation is targeting future codec research activities.

The canonical url for this project is at <https://gitlab.com/AOMediaCodec/SVT-AV1>

## License

Up to v0.8.7, SVT-AV1 is licensed under the BSD-2-clause license and the Alliance for Open Media Patent License 1.0. See [LICENSE](LICENSE-BSD2.md) and [PATENTS](PATENTS.md) for details.
Starting from v0.9, SVT-AV1 is licensed under the BSD-3-clause clear license and the Alliance for Open Media Patent License 1.0. See [LICENSE](LICENSE.md) and [PATENTS](PATENTS.md) for details.

## Documentation

**Quick Start Guides**
- [System Requirements and Build Guide](Docs/System-Requirements.md)
- [Running SVT-AV1 within ffmpeg](Docs/ffmpeg.md)
- [Using the Standalone Encoder](Docs/svt-av1_encoder_user_guide.md)

**Technical Guides**:
- [List of All Paramemeters](Docs/parameters.md)
- [SVT-AV1 Decoder User Guide](Docs/svt-av1_decoder_user_guide.md)

**Common Questions/Issues**
- [What presets do](Docs/CommonQuestions.md#what-presets-do)
- [Scene change detection](Docs/CommonQuestions.md#what-presets-do)
- [GOP size selection](Docs/CommonQuestions.md#gop-size-selection)
- [Threading and efficiency](Docs/CommonQuestions.md#threading-and-efficiency)
- [Practical advice about grain synthesis](Docs/CommonQuestions.md#practical-advice-about-grain-synthesis)
- [Improving decoding performance](Docs/CommonQuestions.md#improving-decoding-performance)
- [Tuning for animation](Docs/CommonQuestions.md#tuning-for-animation)
- [8 vs. 10-bit encoding](Docs/CommonQuestions.md#8-or-10-bit-encoding)
- [HDR and SDR video](Docs/CommonQuestions.md#hdr-and-sdr)
- [Options that give the best encoding bang-for-buck](Docs/CommonQuestions.md#options-that-give-the-best-encoding-bang-for-buck)
- [Multi-pass encoding](Docs/CommonQuestions.md#multi-pass-encoding)
- [CBR, VBR, and CRF modes](Docs/CommonQuestions.md#bitrate-control-modes)

**Technical Appendices**:
- [Adaptive Prediction Structure Appendix](Docs/Appendix-Adaptive-Prediction-Structure.md)
- [Altref and Overlay Pictures Appendix](Docs/Appendix-Alt-Refs.md)
- [CDEF Appendix](Docs/Appendix-CDEF.md)
- [CfL Appendix](Docs/Appendix-CfL.md)
- [Compliant Subpel Interpolation Filter Search Appendix](Docs/Appendix-Compliant-Subpel-Interpolation-Filter-Search.md)
- [Compound Mode Prediction Appendix](Docs/Appendix-Compound-Mode-Prediction.md)
- [Deblocking Loop Filter (LF) Appendix](Docs/Appendix-DLF.md)
- [Film Grain Synthesis](Docs/Appendix-Film-Grain-Synthesis.md)
- [Global Motion Appendix](Docs/Appendix-Global-Motion.md)
- [Intra Block Copy Appendix](Docs/Appendix-Intra-Block-Copy.md)
- [IPP Pass Appendix](Docs/Appendix-IPP-Pass.md)
- [Local Warped Motion appendix](Docs/Appendix-Local-Warped-Motion.md)
- [Mode Decision Appendix](Docs/Appendix-Mode-Decision.md)
- [Motion Estimation Appendix](Docs/Appendix-Open-Loop-Motion-Estimation.md)
- [Overlapped Block Motion Compensation Appendix](Docs/Appendix-Overlapped-Block-Motion-Compensation.md)
- [Palette Prediction Appendix](Docs/Appendix-Palette-Prediction.md)
- [Rate Control Appendix](Docs/Appendix-Rate-Control.md)
- [Recursive Intra Appendix](Docs/Appendix-Recursive-Intra.md)
- [Restoration Filter Appendix](Docs/Appendix-Restoration-Filter.md)
- [SQ Weight Appendix](Docs/Appendix-SQ-Weight.md)
- [Super-resolution Appendix](Docs/Appendix-Super-Resolution.md)
- [Temporal Dependency Model](Docs/Appendix-TPL.md)
- [Transform Search Appendix](Docs/Appendix-TX-Search.md)

**How Can I Contribute?**
- [SVT-AV1 Contribution Guide](Docs/Contribute.md)

