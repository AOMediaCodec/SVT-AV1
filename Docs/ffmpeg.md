[Top level](../README.md)

# Using SVT-AV1 within ffmpeg

**A note on ffmpeg versions**

Although ffmpeg has an SVT-AV1 wrapper, its functionality was severely limited
prior to and including ffmpeg version 5.0. Any version subsequent to that
permits full functionality, including passing SVT-AV1 parameters directly
via the `svt-av1-params` keyword.

If your ffmpeg version is 5.0 or lower, we suggest you upgrade to a more recent version.

The following are some examples of common use cases.

## Most Common Options

The parameters that are most frequently changed from one encode to another are

* `crf`. This parameter governs the quality/size trade-off. Higher crf values will
result in a final output that takes less space, but begins to lose detail. Lower crf
values retain more detail at the cost of file size. The possible range of crf
in SVT-AV1 is 1-63. Crf values are not meant to be equivalent across different
encoders. A good starting point for 1080p video is `crf=30`.
* `preset`. This parameter governs the efficiency/encode-time trade-off. Lower
presets will result in an output with better quality for a given file size, but
will take longer to encode. Higher presets can result in a very fast encode, with
excellent multi-threading, but will make some compromises on visual quality for a
given crf value.
* `-g` (in ffmpeg) or `keyint` (in SvtAv1EncApp). These parameters govern how
many frames will pass before the encoder will add a key frame. Key frames include
all information about a single image. Other (delta) frames store only
differences between one frame and another. Key frames are necessary for seeking
and for error-resilience (in VOD applications). More frequent key frames make the
video quicker to seek and more robust at the cost of efficiency. For VOD, a setting
a key frame once per second or so is a common choice. In other contexts, less
frequent key frames (such as 5 or 10 seconds) are preferred.
* `film-grain`. Because of the random nature of film grain and CCD noise present
in some videos, it is very difficult to compress. The AV1 specification has the
capability to produce synthetic noise of varying strengths. It is therefore
possible for SVT-AV1 to delete film grain and CCD noise and replace it with
[synthetic grain](Appendix-Film-Grain-Synthesis.md) of the same character, resulting in good bitrate savings while
retaining subjective visual quality. The `film-grain` parameter enables this
behavior. Setting it to a higher level does so more aggressively. Very high
levels of denoising can result in the loss of some high-frequency detail, however.
* `pix_fmt`. This parameter forces encoding to 10 or 8 bit color depth. By default
SVT-AV1 will encode 10-bit sources to 10-bit outputs and 8-bit to 8-bit.
* `tune`. This parameter changes some encoder settings to produce a result
that is optimized for subjective quality `tune=0` or PSNR `tune=1`. The former
can result in an apparently sharper image.

## Example 1: Fast/Realtime Encoding

For fast encoding, the preset must be sufficiently high that your CPU can
encode without stuttering. Higher presets are faster but less efficient. The
highest preset is 13.

    ffmpeg -i infile.mkv -c:v libsvtav1 -preset 10 -crf 35 -c:a copy outfile.mkv

Since SVT-AV1V1 is designed to scale well across cores/processors, fast encoding is
best performed on machines with at least 8 logical cores.

## Example 2: Encoding for Personal Use

When encoding for personal use, such as a media server or HTPC, higher efficiency
and *reasonable* encoding times are desirable.

    ffmpeg -i infile.mkv -c:v libsvtav1 -preset 5 -crf 32 -g 240 -pix_fmt yuv420p10le -svtav1-params tune=0:film-grain=8 -c:v copy outfile.mkv

Presets between 4 and 6 offer what many people consider a reasonable trade-off
between quality and encoding time. Encoding with 10-bit depth results in more
accurate colors and fewer artifacts with minimal increase in filesize. The
resulting file may be more computationally intensive to decode in some cases.
If compatibility with older-generation hardware is required, omitting the
`pix_fmt` argument will help. Also, if decode performance is important, passing
the parameter `fast-decode=1` may help (this parameter is
does not have an effect for all presets, so check the [user guide](svt-av1_encoder_user_guide.md)
for your preset.

The `tune=0` parameter optimizes the encode for visual quality (apparent sharpness),
instead of objective quality (PSNR).

The `film-grain` parameter allows SVT-AV1 to detect and delete film grain from the original video,
and replace it with synthetic grain of the same type, resulting in significant bitrate savings. A
value of 8 is a reasonable starting point for live-action video with a normal amount of grain. Higher
values in the range of 10-15 enable more aggressive use of this technique for video with lots of natural
grain. For 2D animation, lower values in the range of 4-6 are often appropriate. If the original
video does not have natural grain, this parameter can be omitted.

Note that the `crf` range for SVT-AV1 is 1-63, which is a wider range than is found on some popular
open-source encoders. As a result, `crf` values that approximate the visual quality in those encoders
will tend to be higher in SVT-AV1.

Using a higher GOP via the `g` parameter results in a more efficient encode, at
the cost of seeking performance. A common rule-of-thumb among hobbyists is to
use ten times the framerate of the video, but not more than 300.

## Example 3: Encoding for Video On Demand

For professional VOD applications, the best possible efficiency is often
desired. For this application, we suggest the use of a third party tool that
will split the video at scene changes as SVT-AV1's `scd` parameter does not
currently insert a key frame at scene changes.

A short GOP size (the `g` parameter) results in better seeking performance and fault-tolerance.

An example use of a single-scene video:

    ffmpeg -i infile.mkv -c:v libsvtav1 -preset 2 -crf 25 -g 24 -pix_fmt yuv420p10le -svtav1-params tune=0:film-grain=8 -c:v copy outfile.mkv

Note that the use of 10-bit color depth can result in decode performance degradation in some cases. Using 8-bit instead will
increase decode performance and compatibility.

# Using an Older Version of ffmpeg

If you are unable to use a version of ffmpeg that is recent enough to pass all required parameters
to SVT-AV1, you can use ffmpeg to decode the video and then pipe the result to the standalone app
for encoding. Then you can add the audio and video into a final file.

    ffmpeg -i infile.mkv -map 0:v:0 -pix_fmt yuv420p10le -f yuv4mpegpipe -strict -1 - | SvtAv1EncApp -i stdin --preset 5 --keyint 240 --input-dept 10 -crf 32 --film-grain 8 -b outfile.ivf
    ffmpeg -i outfile.ivf -i infile.mkv -map 0:v -map 1:a:0 -c:v copy -c:a copy outfile.mkv
