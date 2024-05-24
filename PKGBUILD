# Maintainer: changhaoxuan23
_pkgname=configurationspp
pkgname="$_pkgname-git"
pkgver=automatic
pkgrel=1
pkgdesc="easy to use and powerful command line options parser"
arch=(x86_64)
url="https://github.com/changhaoxuan23/configurationspp"
license=('MIT')
depends=(libc++)
makedepends=(cmake clang ninja)
provides=(configurationspp)
conflicts=(configurationspp)
source=('git+https://github.com/changhaoxuan23/configurationspp.git')
sha256sums=(SKIP)
pkgver() {
  cd "$_pkgname"
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
}
build() {
  cmake -DCMAKE_INSTALL_PREFIX=/usr -G Ninja -B build -S "$_pkgname"
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}