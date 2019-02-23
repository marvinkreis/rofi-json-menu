pkgname=rofi-json-menu
pkgver=0.0.2
pkgrel=1
pkgdesc="Plugin to use rofi for custom menus"
url="https://github.com/marvinkreis/${pkgname}"
arch=("i686" "x86_64")
license=("MIT")
depends=("rofi" "json-c")
makedepends=("git")

source=("git://github.com/marvinkreis/${pkgname}.git")
md5sums=("SKIP")

prepare() {
    cd "$srcdir/rofi-plugins"
    git submodule init
    git submodule update
}

build() {
    cd "$srcdir/rofi-plugins"
    autoreconf -i
    ./configure
    make
}

package() {
    cd "$srcdir/rofi-plugins"
    make DESTDIR="$pkgdir" PREFIX=/usr install
}
