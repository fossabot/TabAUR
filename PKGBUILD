# Maintainer: Toni500 <tonino512@linuxmail.org>
pkgname="tabaur-git"
_pkgname="TabAUR"
pkgver=0.6.4.r18.1712d47
pkgrel=1
pkgdesc="A customizable and lightweight AUR helper, designed to be simple but powerful."
arch=('x86_64' 'aarch64')
url="https://github.com/BurntRanch/TabAUR"
license=('GPL3')
depends=('pacman' 'curl')
makedepends=('cmake' 'make' 'gcc')
optdepends=(
  "sudo: privilege elevation"
  "doas: privilege elevation"
  "git: for using the AUR git repo"
  "tar: for using the AUR tarball repo"
)
source=("git+${url}.git")
sha256sums=("SKIP")

pkgver() {
    cd "${srcdir}/${_pkgname}"
    git describe --long --tags | sed 's/^v//;s/\([^-]*-\)g/r\1/;s/-/./g'
}

prepare() {
    cd "${srcdir}/${_pkgname}"
    git submodule init
    git submodule update --init --recursive
}

build() {
    make -C "${srcdir}/${_pkgname}" CPPFLAGS="-O2 -mtune=generic -funroll-all-loops -march=native -isystem include -std=c++20"
}

package() {
    cd "${srcdir}/${_pkgname}"
    install -Dm755 "taur" "${pkgdir}/usr/bin/taur"
    install -Dm644 "LICENSE" "${pkgdir}/usr/share/licenses/${_pkgname}/LICENSE"
}
