# Summary #

For a several years now ATI proprietary graphic drivers for Linux can't quite catch-up with the development of X and other graphic related projects - in result it was simply not quite feasible to use it on a moderately recent Linux distribution. A partial exception was Ubuntu, yet even there the overall picture was a bit dodgy compared with the nVidia performance and support.

All this is a bit pity considering that ATI are producing some cool hardware and also that on Windows there are no such troubles.


# Details #

Toped is an openGL tool which relies heavily on a stability and performance of the graphic platform underneath - so as a consequence of the problems described above for quite some time I haven't seen it working as expected on Linux platform with ATI graphic card. The open source [radeon](http://dri.freedesktop.org/wiki/Radeon)  driver did a great progress recently and now even the VBO renderer is working. The trouble there are the [stipple patterns](http://code.google.com/p/toped/wiki/StipplePattersMesa) we're using. They are still not supported - in result all fill patterns are visualised as solid.

With the release of the [Gnome 3](http://www.gnome.org/gnome-3) the things got even worse, but in a bizarre way it actually provoked some activity in AMD and for the first time in a couple of years I saw a Catalyst driver actually installed from yum and working along with the latest Fedora packages.

Well... almost working. Here are some sporadic observations of the graphic performance with [Catalyst](http://www.amd.com/us/products/technologies/amd-catalyst/pages/catalyst.aspx) 11.9 installed from [rpmfusion](http://rpmfusion.org/) packages.

First impresions:
  * fgl\_glxgears
    * without Desktop effects - tops 1600 FPS
    * with Desktop effects activated - around 350 FPS max. Interestingly enough - if I simply move the mouse around it seems to improve the performance up to 600 FPS.
  * on K desktop the whole thing feels a bit unstable. It tends to crash - for example if you switch off desktop effects.
  * The original radeon package deals quite well with multi screen configurations, virtually without a need for additional set-up. The Catalyst kind of insist on cloned display configuration. One can run the Catalyst Control Center and change this, but
    1. you have to restart X to activate your new configuration
    1. it returns to a cloned display if you restart the machine

Toped behaves strange on this background.

  1. **Without desktop effects** - with both renderers, the driver crashes on a regular basis. Even in the short periods when it holds - a strange "effects" can be observed - most notably a flashing exchanges between the current and previous screen images. The impression is that some kind of troubles with the back buffers are forcing the driver to do this.
  1. **With desktop effects on** - it looks stable. No flashing and no crashes observed. However
    * with the VBO renderer - there are virtually no graphic objects on the screen apart from the texts. Grids and all temporary objects can be seen without a problem. It appears that there is a VBO problem although fonts are using the same VBO mechanism - true in a bit different way.
    * **with the basic renderer** - I'm afraid to say - **it looks like working**.

All the above does not come as a big surprise - a plenty of bug reports can be found on the net. So is there a

# Light at the end of the tunnel ? #

It looks like there are some new versions of the driver in the pipe. According to [this](http://phoronix.com/forums/showthread.php?64252-AMD-Catalyst-11.10-Linux-Driver-Released/page2) post version 11.10 has been released although on the [official AMD site](http://support.amd.com/us/gpudownload/linux/Pages/radeon_linux.aspx) I still can see only 11.9

Another post in the same forum claims that [AMD Catalyst 11.12 Will Be Even Better](http://phoronix.com/forums/showthread.php?64399-AMD-Catalyst-11.12-Will-Be-Even-Better). Not sure though the comments share its optimism.

On the other hand [Fedora 16 (Verne)](http://fedoraproject.org/get-fedora) just got out of the oven and not surprisingly there are reports that the latest Catalyst driver does not work with it because of the new X version.

Despite the bitterness I'll have to say that Toped will continue to have troubles on this particular platform for the foreseeable future. And not much we can do about it...