# DMultiTool Notifications

Android 8+ notification listener + paired BLE client. GPL-3.0-only.
Setup: enable Receiver on DMultiTool, pair the phone, install the APK,
grant Bluetooth and Notification Access, choose the device, send a test.
Global popups and Wake screen are independent device settings.

The app has no Internet permission. Notifications only go to the selected
paired BLE device. Disable forwarding in the app, disable Receiver on the
multitool, or revoke Notification Access to stop it.

Build prerequisites: JDK, Android API 35, Android build-tools 35, unzip/zip.
No Gradle/network is needed after the SDK is installed:

```sh
DMULTITOOL_ANDROID_SDK=/path/to/sdk bash companion/android/build.sh
```

Output: `build/android/DMultiTool-Notifications.apk`. The build script creates
an ignored local development signing key. Keep that key if you want to install
updates over this build; otherwise uninstall the old APK first.
APK compilation/signature checks do not verify a real Android BLE connection.
