If you're getting this in a release tarball with a configure script:

Build (including initial loader - requires SDCC):

    ./configure
    make

Build (without initial loader):

    ./configure --disable-coldboot
    make

If there is no configure (i.e. you got the code directly from the repo), there is an additional steps to do first.

The build system makes use of automake, autoconf and libtool.  Make sure
they're installed on your system:

    sudo apt-get install automake autoconf libtool

Generate the build files:

    autoreconf -i

