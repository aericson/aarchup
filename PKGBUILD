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
md5sums=('9e4dc9b63f99ba4cbf164a4c9068cab8')
optdepends=('cower: AUR support(--aur)')

build() {
	cd $pkgname-$pkgver/src
	autoconf || return 1
	./configure || return 1
	make || return 1
	make DESTDIR=$pkgdir install
}
