# Contributor: Rorschach <r0rschach@lavabit.com>
# Contributor: Andrew Kravchuk <awkravchuk@gmail.com>
# Contributor: aericson <de.ericson@gmail.com>

pkgname=aarchup
pkgver=1.5
pkgrel=1
pkgdesc="Fork of archup a small and lightweight update-notifier for archlinux."
url="https://github.com/aericson/aarchup"
arch=('i686' 'x86_64')
license="GPL"
depends=('pacman' 'libnotify')
makedepends=('libnotify' 'autoconf' 'gzip')
source=(https://github.com/downloads/aericson/aarchup/$pkgname-$pkgver.tar.gz)
md5sums=('f651ce7f513b6d65ad64682e007da892')

build() {
	cd $pkgname-$pkgver/src
	autoconf || return 1
	./configure || return 1
	make || return 1
	make DESTDIR=$pkgdir install
}
