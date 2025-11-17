pkgname=package-explorer
pkgver=0.1.0.ed5aead
pkgrel=1
pkgdesc="TUI package explorer for Pacman (packages, deps, AUR flags, fuzzy search)"
arch=('x86_64')
url="https://github.com/leugard21/package-explorer"
license=('MIT')
depends=('ncurses' 'pacman')
makedepends=('git' 'cmake')
source=("package-explorer::git+https://github.com/leugard21/package-explorer.git")
sha256sums=('SKIP')

pkgver() {
  cd "$srcdir/package-explorer"

  local ver
  ver="$(git describe --tags --long 2>/dev/null | sed 's/^v//;s/-/./g')"

  if [[ -z "$ver" ]]; then
    local commit
    commit="$(git rev-parse --short HEAD)"
    ver="0.1.0.${commit}"
  fi

  printf "%s" "$ver"
}

build() {
  cd "$srcdir/package-explorer"
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
  cmake --build build
}

package() {
  cd "$srcdir/package-explorer"

  install -Dm755 "build/package-explorer" "$pkgdir/usr/bin/package-explorer"
}
