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
between 7 and 13 are used for fast and real-time encoding. One should use
the lowest preset that is tolerable.

# Scene Change Detection

Modern video files include key frames, which save the entire image, and delta
frames, which store only information changed since the last key frame. A scene
change can be a reasonable time to insert a new key frame since the previous
key frame may not have much in common with subsequent images.

At present, SVT-AV1 does not insert key frames at scene changes. The `scd`
parameter does not change the encoding. It is, therefore, advisable to use
a third-party splitting program to encode videos by chunks if key frame
insertion at scene changes is desired.

# GOP Size Selection

Modern video files include key frames, which save the entire image, and delta
frames, which store only information changed since the last key frame. The GOP
size (governed by the `keyint` parameter) determines how frequently a key frame
is inserted. In general, key frames are far larger than delta frames, so using a
larger GOP can significantly improve efficiency.

However, frequent key frames can be helpful in two respects:

1. They represent seek times. Decode software must go to the nearest previous
   key frame when a seek is requested, then it will need to process all delta
   frames from that key frame until the desired time. Closely spaced
   key frames make seeking much faster.
2. In video on demand applications, key frames allow improved resilience to
   lost packets as each key frame serves as a sort of reset for the image. If
   the key frames are too widely spaced, missed packets can cause relatively
   long-lasting visual artifacts.

For video on demand applications, it is common to use GOP sizes of approximately
one second. For example `keyint=24` for a 24 fps video. Home users frequently prefer
significantly longer GOP sizes of 5-10 seconds.

# Threading and Efficiency

SVT-AV1 is specifically designed to scale well across many logical processors.
Some multi-threading techniques do not cause a decrease in resulting
quality/efficiency while others do. At lower presets, multi-threading is
disabled on tools where using it will reduce quality. As result, the number
of cores SVT-AV1 can usefully use varies by preset.

For presets in the range of 4-6, SVT-AV1 seems to fully utilize at least 16 logical
processors relatively completely.

# Practical Advice on Grain Synthesis

The random pattern associated with film grain (or CCD noise) is notoriously difficult
to compress. When grain is present in the original source, significant efficiency gains
can be made by deleting film grain and replacing it with synthetic grain that
does not take up significant space in the file. When `film-grain` is enabled, SVT-AV1
deletes the existing grain, compares the resulting image with the original, analyzes
the nature of the grain, and reproduces it with synthetic grain. This can greatly
improve efficiency while retaining visual quality.

The process of removing grain can sometimes delete very fine detail from the
original image, so it should not be used too aggressively. The level passed
to the `film-grain` parameter controls how aggressively this procedure is
employed.

As a general rule, a `film-grain` level of around 8 is sufficient for
live action video with a normal amount of grain. Noisier video benefits
from higher levels in the 10-15 range.  2-D animation typically has less grain,
so a level of around 4 works well with standard (hand-drawn) animation. Grainy
animation can benefit from higher levels up to around 10.

# Improving Decoding Performance

Although modern AV1 decoders (such as dav1d and hardware decoders) are extremely
efficient, there can be cases where older hardware has a difficult time decoding
AV1 streams without stuttering.

Tips that may improve decoding performance include:

* Use of the `fast-decode=1` parameter
* Use of 8-bit color depth instead of 10
* Reduction in the bitrate used (higher CRF values mean lower bitrate)
* Avoiding the use of synthetic grain

# 8 or 10-bit Encoding

TODO

# Options That Give the Best Encoding Bang-For-Buck

TODO

# Multi-Pass Encoding

Some encoders benefit from the use of a multi-pass encoding approach. In SVT-AV1,
in general, multi-pass encoding is useful for achieving a target bitrate when
using VBR (variable bit rate) or CBR (constant bit rate) encoding. When using
CRF (constant visual rate factor) mode, there is little to no advantage to
doing a multi-pass encode.

# Bitrate Modes

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
