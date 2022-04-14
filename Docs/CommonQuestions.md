[Top level](../README.md)

# What Presets Do

Presets control how many efficiency features are used during the encoding
process, and the intensity with which those features are used. Lower presets
use more features and produce a more efficient file (smaller, for a given
quality). However, lower presets also require more compute time during the
encode process. If a file is to be widely distributed, it can be worth it to
use very low presets, while high presets allow very fast encoding, such
as for real-time applications.

Generally speaking, presets 1-3 represent extremely high efficiency, for
use when encode time is not important and quality/size of the resulting
video file is critical. Presets 4-6 are commonly used by home enthusiasts
as they represent a balance of efficiency and reasonable compute time. Presets
between 7 and 12 are used for fast and real-time encoding. Preset 13 is even
faster but not recommended for videos meant for human consumption. One should
use the lowest preset that is tolerable.

# Scene Change Detection

Modern video files include key frames, which are intra-coded pictures, and
inter-frames, which store only information changed since the previously encoded
reference frames. A scene change can be a reasonable time to insert a new key
frame since the previous key frame may not have much in common with subsequent
images.

At present, SVT-AV1 does not insert key frames at scene changes, regardless of
the `scd` parameter. It is, therefore, advisable to use a third-party splitting
program to encode videos by chunks if key frame insertion at scene changes is
desired. 

Users who are unable to perform their own key frame insertion should consider
the use of subjective mode (`--tune 0`). In subjective mode, when a new scene 
is detected, encoding improvements are made to the most important pictures at
the start of the new scene to accommodate the new scene or other non-motion 
changes.

# GOP Size Selection

Modern video files include key frames, which save the entire intra-coded image,
and delta frames, which store only information changed since the last key frame
(through motion-compensated-prediction and residual coding). The GOP size
(governed by the `keyint` parameter) determines how frequently a key frame is
inserted. In general, key frames are far larger in terms of bits than delta
frames, so using a larger GOP can significantly improve efficiency.

However, frequent key frames can be helpful in two respects:

1. They make seek times much faster. Decode software must go to the nearest
   previous key frame when a seek is requested, then it must process
   all delta frames between the key frame and the desired seek point. When key
   frames are far apart, seek performance can suffer.
2. In video on demand applications, key frames provide improved resilience to
   lost packets. Each key frame serves as a sort of reset for the reference
   chain used by inter frames. If the key frames are too widely spaced, missed
   packets can cause relatively long-lasting visual artifacts.

For video on demand applications, it is common to use GOP sizes of approximately
one second. For example `keyint=24` for a 24 fps video. Home users frequently prefer
significantly longer GOP sizes of 5-10 seconds.

# Threading and Efficiency

SVT-AV1 is specifically designed to scale well across many logical processors.
By default, SVT-AV1 only uses multi-threading techniques that do not cause a
decrease in the resulting quality/efficiency (for example, it does not use
tile-based threading, which is known to decrease quality). As a result, the
quality of the video will be the same when using `--lp 1` as `--lp n` in the
default CRF configuration. Although SVT-AV1 supports tiles, it does not rely 
on tiles for parallelization.

Anecdotally, SVT-AV1 is able to fairly efficiently about 16 logical cores.
Beyond that, its ability to fully utilize cores drops off. For this reason, 
tools that split video into scene-based chunks can be useful if greater 
multiprocessing is desired.

Note that the highest quality presets (0-3) use features that have a lot of
dependencies, which may lead to lower CPU usage. 

# Practical Advice on Grain Synthesis

The random pattern associated with film grain (or CCD noise) is notoriously difficult
to compress. When grain is present in the original source, significant efficiency gains
can be made by deleting film grain and replacing it with synthetic grain that
does not take up significant space in the file. When `film-grain` is enabled, SVT-AV1
denoises the image, compares the resulting image with the original, analyzes
the nature of the grain. It then inserts synthetic grain with similar
properties. This can greatly improve efficiency while retaining visual quality.

The process of removing grain can sometimes delete very fine detail from the
original image, so it should not be used too aggressively. The level passed
to the `film-grain` parameter controls how aggressively this procedure is
employed.

As a general rule, a `film-grain` level of around 8 is sufficient for
live action video with a normal amount of grain. Noisier video benefits
from higher levels in the 10-15 range.  2-D animation typically has less grain,
so a level of around 4 works well with standard (hand-drawn) animation. Grainy
animation can benefit from higher levels up to around 10.

More detail on film grain synthesis is available in the [appendix](Appendix-Film-Grain-Synthesis.md).

# Improving Decoding Performance

Although modern AV1 decoders (such as dav1d and hardware decoders) are
extremely efficient, there can be cases where software decoders on slower
hardware have a difficult time decoding AV1 streams without stuttering.

Tips that may improve decoding performance include:

* Use of the `fast-decode=1` parameter
* Use of 8-bit color depth instead of 10
* Reduction in the bitrate used (higher CRF values mean lower bitrate)
* Avoiding the use of synthetic grain

# Tuning for Animation

There are two types of video that are called "animation": hand-drawn 2D
and 3D animated. Both types tend to be easy to encode (meaning the resulting
file will be small), but for different reasons. 2D animation often has large
areas that do not move so that the difference between one frame and another is
small in many cases. In addition, it tends to have small amounts of grain.
Experience has shown that relatively high `crf` values with low levels of
`film-grain` produce 2D animation results that are visual very close to the
original.

3D animation has much more detail and movement, but it sometimes has no grain
whatsoever, or only small amounts that were purposely added to the image. Encoding
3D animation with no grain will go much faster if `film-grain` synthesis is not
enabled at encode time.

# 8 or 10-bit Encoding

Video may be encoded with either 8 or 10-bit color depth. 10-bit video
can represent more colors and is less prone to certain artifacts, such as color
banding and loss of detail in low luma areas. Most SDR sources come in 8-bit
color and SVT-AV1, by default, will encode 8-bit video to 8-bit video or 10-bit
to 10-bit.

It is possible to encode 8-bit video to a 10-bit final result. This allows the
encoder to use less rounding and can produce slightly better fidelity. There is
a small cost in terms of resulting file size (~5%), and, with some encoders, an
encoding-time cost. SVT-AV1 was carefully designed to be able to encode 10-bit
color in a compute-efficient manner, so there should not be much of a
encoding performance penalty associated with 10-bit except at faster presets
(11-13), where the slowdown may be more noticeable. One should be aware,
however, that 10-bit *decoding* can also be more compute-intensive than 8-bit
in some cases.

It is possible to encode 10-bit video to an 8-bit result. This may make sense
if broad compatibility is critical and a small loss in fidelity is acceptable.
Many users are unable to tell the difference between 10 and 8-bit SDR video.

To force the final result to be 10-bit, specify `-pix_fmt yuv420p10le` in ffmpeg, or you
can force 8-bit using `-pix_fmt yuv420p` in ffmpeg.

# HDR and SDR Video

Some video sources now come designed for display on high dynamic range (HDR) hardware.
SVT-AV1 can encode these sources correctly and embed the required metadata in the final
output. High definition typically involves a high definition color space (such as BT.2020)
and an associated set of transfer functions (such as pr or HLG). HDR video is also
typically encoded with 10-bit color. Detailed information on forcing these
settings is available in the [full parameters description](parameters.md).

Standard dynamic range video is usually encoded in a BT.709 color space. Colors
can be mapped from a wider space into a less wide space using ffmpeg or other
tools, but there are always losses, either in the accuracy of the color or
the retention of detail in high and low brightness areas. As a rule, it is best not
avoid conversion from HDR to SDR, if possible.

# Options That Give the Best Encoding Bang-For-Buck

The quality/filesize tradeoff is controlled by the `crf` parameter. Increasing
this parameter can significantly reduce the file size. AV1 is very efficient at
preserving the types of details that humans notice, so significant reduction in
objective quality (PNSR and similar measures) can still result in a video with
good subjective quality.

[Film grain synthesis](#practical-advice-on-grain-synethsis) can also significantly
reduce file size while retaining apparent visual quality. It is not enabled by
default because not all sources have film grain or CCD noise, Additionally,
the de-noising process used in this procedure can delete very fine details
(such as small, fast-moving particles or skin imperfections), so the
aggressiveness of the denoising/synthesis needs to be paired with the strength
of the grain/noise in the original sample.

# Multi-Pass Encoding

Some encoder features benefit from or require the use of a multi-pass encoding
approach. In SVT-AV1, in general, multi-pass encoding is useful for achieving a
target bitrate when using VBR (variable bit rate) encoding, although both
one-pass and multi-pass modes are supported.

When using CRF (constant visual rate factor) mode, multi-pass encoding is
designed to improve quality for corner case videos--it is particularly helpful
in videos with high motion because it can adjust the prediction structure (to
use closer references, for example). Multi-pass encoding, can, therefore, be
said to have an impact on quality in CRF mode, but is not crticial in most
cases. In general, multi-pass encoding is not as important for SVT-AV1 in CRF
mode than it is for some other encoders.

CBR (constant bit rate) encoding is always one-pass.

# Bitrate Control Modes

SVT-AV1 supports three general approaches to controlling the bitrate.

* In CBR (constant bit rate) mode, the target bitrate is forced at all times.
This results in a predictable file size, but inefficient use of space as simple
scenes get too many bits and complex scenes get too few. Use `--rc=2` for this mode.
* In VBR (variable bit rate) mode, a target bitrate it set, but the effective bitrate
can vary above and below the target. Use `--rc=1` to enable VBR mode and set
the flexibility of the effective bitrate using `--bias-pct`. A value closer to
zero makes the encode behave more like a CBR encode, while a value closer to
100 gives it greater flexibility.
* CRF (constant rate factor) mode targets a constant visual quality. This approach
leads to a favorable visual quality for a given file size and is recommended
for applications where a target bitrate is not necessary, such as in a home environment.
Set `--rc=0` to use this method.
