# System dependencies

The audit collector requires common local tools:

- bash
- git
- find
- grep
- sed
- awk
- tar
- sha256sum
- wc
- date

For build reproduction, reviewers may also need:

- JDK compatible with the configured Android Gradle Plugin
- repository Gradle wrapper
- Android SDK
- Android NDK
- CMake
- Ninja or Make
- adb
- Android device or emulator

The collector records detected versions where possible. It does not install
dependencies, change system configuration, build the app, run tests, reset git,
or modify gameplay/source files.
