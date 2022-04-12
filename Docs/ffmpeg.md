# Using SVT-AV1 within ffmpeg

**A note on ffmpeg versions** 

Although ffmpeg had an SVT-AV1 plugin, its functionality was severely limited
prior to and including ffmpeg version 5.0. Any version subsequent to that
permits the full functionality, including passing SVT-AV1 parameters directly
via the `svt-av1-params=` keyword. 

If your ffmpeg version is 5.0 or lower, we suggest you upgrade to a more recent version.

The following are some examples of common use cases:

## Example 1: Fast/Realtime Encoding

For fast encoding, the preset must be sufficiently high that your CPU can
encode without stuttering. Higher presets are faster but less efficient. The
highest preset is 13.

    ffmpeg -i infile.mkv -c:v libsvtav1 -preset 10 -crf 35 -c:a copy outfile.mkv

Since SVTAV1 is designed to scale well across cores/processors, fast encoding is
best performed on machines with at least 8 logical cores.

## Example 2: Encoding for Personal Use

When encoding for personal use, such as a plex server or HTPC, higher efficiency 
and *reasonable* encoding times are desirable. 

    ffmpeg -i infile.mkv -c:v libsvtav1 -preset 5 -crf 32 -g 240 -pix_fmt yuv420p10le -svtav1-params tune=0:film-grain=8 -c:v copy outfile.mkv

Presets between 4 and 6 offer what many people consider a reasonable trade-off
between quality and encoding time. Encoding with 10-bit depth results in more
accurate colors and fewer artifacts with minimal increase in filesize. The
resulting file may be more computationally intensive to decode in some cases.
If compatibility with older-generation hardware is required, omitting the
`pix_fmt` argument will help. Also, if decode performance is important, passing
the parameter `fast-decode` with an appropriate level may help (this parameter is
not available for all presets, so check the [user guide](svt-av1_encoder_user_guide.md)
for the levels avaialable for your preset.

The `tune=0` parameter optimizes the encode for visual quality (apparent sharpness),
instead of objective quality (PSNR).

The `film-grain` parameter allows SVT-AV1 to detect and delete film grain from the original video,
and replace it with synthetic grain of the same type, resulting in significant bitrate savings. A
value of 8 is a reasonable starting point for live-action video with a normal amount of grain. Higher
values in the range of 10-15 enable more aggressive use of this technique for video with lots of natural
grain. For 2D animation, lower values in the range of 4-6 are often appropriate. If the original
video does not have natural grain, this parameter can be omitted.

Note that the CRF range for SVTAV1 is 0-63, which is a wider range than is found on some popular
open-source encoders. As a result, CRF values that approximate the visual quality in those encoders
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

An example use of a single-scene video 

    ffmpeg -i infile.mkv -c:v libsvtav1 -preset 2 -crf 25 -g 24 -pix_fmt yuv420p10le -svtav1-params tune=0:film-grain=8 -c:v copy outfile.mkv

Note that the use of 10-bit color depth can result in decode performance degradation in some cases. Using 8-bit instead will 
increase decode performance and compatibility.

# Using an Older Version of ffmpeg

If you are unable to use a version of ffmpeg that is recent enough to pass all required parameters
to SVT-AV1, you can use ffmpeg to decode the video and then pipe the result to the standalone app
for encoding. Then you can add the audio and video into a final file.

    ffmpeg -i infile.mkv -map 0:v:0 -pix_fmt yuv420p10le -f yuv4mpegpipe -strict -1 - | SvtAv1EncApp -i stdin --preset 5 --keyint 240 --input-dept 10 -crf 32 --film-grain 8 -b outfile.ivf
    ffmpeg -i outfile.ivf -i infile.mkv -map 0:v -map 1:a:0 -c:v copy -c:a copy outfile.mkv
