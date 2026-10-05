#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
sdk="${DMULTITOOL_ANDROID_SDK:-/tmp/dmt-android-sdk}"
platform="$(find "$sdk" -name android.jar -print -quit)"
tools="$(find "$sdk" -name aapt -print -quit)"
[[ -n "$platform" && -n "$tools" ]] || { echo 'Set DMULTITOOL_ANDROID_SDK to Android SDK with API 35 and build-tools 35.' >&2; exit 1; }
tools="$(dirname "$tools")"
out="$PWD/build/android"
mkdir -p "$out/classes" "$out/dex"
javac --release 8 -classpath "$platform" -d "$out/classes" companion/android/src/org/dmultitool/notifications/*.java
"$tools/d8" --lib "$platform" --min-api 26 --output "$out/dex" "$out"/classes/org/dmultitool/notifications/*.class
"$tools/aapt" package -f -M companion/android/AndroidManifest.xml -I "$platform" -F "$out/unsigned.apk"
(cd "$out/dex" && zip -q "$out/unsigned.apk" classes.dex)
"$tools/zipalign" -f 4 "$out/unsigned.apk" "$out/aligned.apk"
if [[ ! -f "$out/debug.keystore" ]]; then
  keytool -genkeypair -keystore "$out/debug.keystore" -storepass android -keypass android -alias dmultitool -keyalg RSA -keysize 2048 -validity 10000 -dname 'CN=DMultiTool development build' -noprompt
fi
"$tools/apksigner" sign --ks "$out/debug.keystore" --ks-pass pass:android --key-pass pass:android --out "$out/DMultiTool-Notifications.apk" "$out/aligned.apk"
"$tools/apksigner" verify "$out/DMultiTool-Notifications.apk"
echo "$out/DMultiTool-Notifications.apk"
