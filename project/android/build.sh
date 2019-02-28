#! /bin/sh

set -e

./gradlew clean assembleRelease
rm -rf  /d/JIE/workspace/VCloudBox/app/libs/libmedia-release.aar
mv libmedia/build/outputs/aar/libmedia-release.aar   /d/JIE/workspace/VCloudBox/app/libs/libmedia-release.aar
