#!/bin/bash

INDIR=$1
SOURCEDIR=$2
TARGET_DIR=$3
VERSION=$4
OSMOD=$5


# releases are X.Y.Z; nightlies sort above the newest release tag and below the next release
TAG_PREFIX=v
if [[ ${VERSION} =~ ^(${TAG_PREFIX})?([0-9]+\.[0-9]+\.[0-9]+)$ ]]; then
    DEB_VERSION=${BASH_REMATCH[2]}
else
    BASE=${DEB_BASE_VERSION:-0.0.0}
    if ! [[ ${BASE} =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "ERROR: DEB_BASE_VERSION '${BASE}' is not X.Y.Z"
        exit 1
    fi
    LAST_REL=$(git -C "${SOURCEDIR}" tag --list "${TAG_PREFIX}*" \
        | sed -nE "s/^${TAG_PREFIX}([0-9]+\.[0-9]+\.[0-9]+)$/\1/p" | sort -V | tail -n 1)
    COUNT=$(git -C "${SOURCEDIR}" rev-list --count HEAD || echo 0)
    DATE=$(TZ=UTC git -C "${SOURCEDIR}" log -1 --format=%cd --date=format-local:%Y%m%d || date -u +%Y%m%d)
    HASH=$(git -C "${SOURCEDIR}" rev-parse --short HEAD || echo nohash)
    if [[ $(git -C "${SOURCEDIR}" rev-parse --is-shallow-repository) == true ]]; then
        echo "WARNING: shallow clone; nightly deb versions will not order correctly"
    fi
    if [[ -z ${LAST_REL} ]] || dpkg --compare-versions "${BASE}" gt "${LAST_REL}"; then
        DEB_VERSION="${BASE}~nightly.${DATE}.${COUNT}.${HASH}"
    else
        DEB_VERSION="${LAST_REL}+nightly.${DATE}.${COUNT}.${HASH}"
    fi
fi
if ! [[ ${DEB_VERSION} =~ ^[0-9][A-Za-z0-9.+~]*$ ]]; then
    echo "ERROR: '${DEB_VERSION}' is not a valid deb version"
    exit 1
fi
echo "DEB VERSION is ${DEB_VERSION}"

mkdir ./installer-tmp
pushd ./installer-tmp

# Names
PLUG_NAME=OB-Xf
# debian package names must be lower case
PACKAGE_NAME="ob-xf"

# locations

# Cleanup from failed prior runs
rm -rf ${PACKAGE_NAME} product
mkdir -p ${PACKAGE_NAME}/usr/lib/vst3
mkdir -p ${PACKAGE_NAME}/usr/lib/lv2
mkdir -p ${PACKAGE_NAME}/usr/lib/clap
mkdir -p ${PACKAGE_NAME}/usr/bin
mkdir -p ${PACKAGE_NAME}/usr/share/Surge\ Synth\ Team/${PLUG_NAME}/doc
mkdir -p ${PACKAGE_NAME}/DEBIAN
chmod -R 0755 ${PACKAGE_NAME}

# build control file

if [[ -f ${PACKAGE_NAME}/DEBIAN/control ]]; then
	rm ${PACKAGE_NAME}/DEBIAN/control
fi
touch ${PACKAGE_NAME}/DEBIAN/control
cat <<EOT >> ${PACKAGE_NAME}/DEBIAN/control
Source: ${PACKAGE_NAME}
Package: ${PACKAGE_NAME}
Version: $DEB_VERSION
Architecture: $(dpkg --print-architecture)
Maintainer: surgeteam <noreply@github.com>
Depends: libc6 (>=2.35), libstdc++6 (>=12), libgcc-s1, libfontconfig1, libfreetype6, libasound2, libx11-6, libxext6, libxcursor1, libxinerama1, libxrandr2
Recommends: xdg-utils, libxss1, zenity | kdialog
Provides: vst-plugin
Section: sound
Priority: optional
Sndstation-Kind: clap-plugin
Description: Virtual Analog Synth
EOT

cat ${PACKAGE_NAME}/DEBIAN/control

touch ${PACKAGE_NAME}/usr/share/Surge\ Synth\ Team/${PLUG_NAME}/doc/changelog.Debian
DATE=`date --rfc-email`
MSG=`git log -n 1 --pretty="%s (git hash %H)"`
cat <<EOT > ${PACKAGE_NAME}/usr/share/Surge\ Synth\ Team/${PLUG_NAME}/doc/changelog.Debian
${PACKAGE_NAME} (${DEB_VERSION}) stable; urgency=medium

  * ${MSG}
  * For more details, see: https://github.com/surge-synthesizer/surge

 -- Surge Synthesizer Team <noreply@github.com>  ${DATE}
EOT
gzip -9 -n ${PACKAGE_NAME}/usr/share/Surge\ Synth\ Team/${PLUG_NAME}/doc/changelog.Debian

#copy data and vst plugins

cp ${SOURCEDIR}/LICENSE ${PACKAGE_NAME}/usr/share/Surge\ Synth\ Team/${PLUG_NAME}/doc/
cp -r ${SOURCEDIR}/assets/installer/Surge\ Synth\ Team/OB-Xf/* ${PACKAGE_NAME}/usr/share/Surge\ Synth\ Team/${PLUG_NAME}/

# Copy the VST3 bundle
cp -r "${INDIR}/OB-Xf.vst3" ${PACKAGE_NAME}/usr/lib/vst3/
cp -r "${INDIR}/OB-Xf" ${PACKAGE_NAME}/usr/bin/

# set permissions on shared libraries
find ${PACKAGE_NAME}/usr/lib/vst3/ -type f -iname "*.so" -exec chmod 0644 {} +

cp -r "${INDIR}/OB-Xf.clap" ${PACKAGE_NAME}/usr/lib/clap/
cp -r "${INDIR}/OB-Xf.lv2" ${PACKAGE_NAME}/usr/lib/lv2/

echo "----- LIBRARY CONTENTS -----"
find ${PACKAGE_NAME}/usr/lib -print
find ${PACKAGE_NAME}/usr/bin -print
find ${PACKAGE_NAME}/usr/share -print

# build deb package
dpkg-deb --verbose --build ${PACKAGE_NAME} ${TARGET_DIR}/ob-xf-Linux${OSMOD}-${VERSION}.deb

# create a tarball of the {PACKAGE_NAME}/usr contents.
pigz="$(command -v pigz)"

pushd ${PACKAGE_NAME}/usr
    # tar cf - ./* | "${pigz:-gzip}" > "${TARGET_DIR}/${PACKAGE_NAME}-linux-x86_64-${VERSION}.tar.gz"
popd


rm -rf ${PACKAGE_NAME}
popd

