# PKGBUILD
#
# Maintainer: Greg Mack <greg@web.net>

pkgname=spooky-scoreboard-daemon
pkgdesc="Spooky Scoreboard Daemon for your Spooky pinball machine"
pkgver=0.0.2
pkgrel=1
arch=('x86_64')
url=https://github.com/gregcube/spooky_scoreboard_daemon
license=('GPL3')

depends=('curl' 'libx11' 'libxft' 'libxpm' 'fontconfig' 'gcc-libs' 'glibc')

build() {
  cd ..
  cmake -B build
  cmake --build build
}

package() {
  cd ..
  install -Dm755 build/ssbd ${pkgdir}/usr/bin/ssbd
}
