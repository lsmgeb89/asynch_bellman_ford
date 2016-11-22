#!/bin/bash
#

readonly NAME="cs6380_project_2"
readonly RELEASE_FOLDER="${HOME}/${NAME}"
readonly RELEASE_TAR="${HOME}/${NAME}.tar.gz"

# delete previous release tar
if [ -f "$RELEASE_TAR" ]; then
  rm "$RELEASE_TAR"
fi

mkdir -p "$RELEASE_FOLDER/src"
mkdir -p "$RELEASE_FOLDER/test"
# copy source files
cp -ar src/* "$RELEASE_FOLDER"/src
cp -ar test/* "$RELEASE_FOLDER"/test
# copy readme and scripts
cp readme.txt build_run*.sh "$RELEASE_FOLDER"
# package all files
pushd "${HOME}" > /dev/null 2>&1
tar -czvf "$RELEASE_TAR" "$NAME"
popd > /dev/null 2>&1

# delete release folder
if [ -d "$RELEASE_FOLDER" ]; then
  rm -rf "$RELEASE_FOLDER"
fi

