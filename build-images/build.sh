#!/bin/bash

set -e
set -x

BUILD_ROOT='/tmp/webcom-build/'
PACKAGE_DEST='/tmp/webcom-package/'

git_url=${git_url:-https://github.com/webcom-components/webcom-sdk-c.git}
git_ref=${git_ref:-master}
package_flavour="${package_flavour:-0}"

if cmake --version ; then
	CMAKE=cmake
else
	if cmake3 --version ; then
		CMAKE=cmake3
	else
		exit 1
	fi
fi

mkdir -p "$BUILD_ROOT"

cd "$BUILD_ROOT"

repo=$(basename "$git_url" .git)

if [ -d "$repo" ] ; then
	cd "$repo"
	git fetch
else
	git clone "$git_url"
	cd "$repo"
fi

git checkout "$git_ref"
git merge "origin/$git_ref"

cd build

"$CMAKE" .. "-DPACKAGE_FLAVOUR=${package_flavour}" "-DCPACK_OUTPUT_FILE_PREFIX=$PACKAGE_DEST" "$@"
make clean all
make package
