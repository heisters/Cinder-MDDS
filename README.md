MDDS block for Cinder
=====================

A Cinder block for playback of sequences of DDS-compressed textures. Like
M-JPEG, but with DDS.

Usage
-----

First, you'll need a directory full of DDS-compressed textures. The easiest way
to do this is probably to use something like FFMPEG or Handbrake to encode a
video into a PNG image sequence, and then use ATI's Compressonator (or another
DDS batch-encoder) to turn the PNGs into DDS files. Unfortunately, I'm not
aware of a good way to do the DDS compression on OSX (see TODO below).

Once encoding is done, look at the sample application. Basically, you just
point `mdds::Movie` at your folder of DDS textures, and from there on it works
just like any other movie playing class.

Rationale
---------

Using DDS texture sequences for movie playback uses very little system memory
and very little CPU. Furthermore, using this library allows movie playback
without QuickTime, which, in turn, allows compiling Cinder apps in 64-bit mode,
which can be useful if you're doing work that involves storing lots of data in
memory (like, say, large buffers of video frames).

On the other hand, DDS texture sequences have abysmal compression ratios when
compared to real video formats, so they'll not only use a lot of disk space,
but the disk I/O will become a bottleneck unless you have an SSD drive.

This library is very similar to the excellent and ambitious
[Hap](https://github.com/Vidvox/hap). Hap, however, is currently limited to
playback using QuickTime (see above). Furthermore, the simplicity of this
library makes it easy for you to fix it, explore it, and hack it.

TODO
----

* Add support for YCoCg (DXT-6) compressed textures
* Add an application for transcoding movies
