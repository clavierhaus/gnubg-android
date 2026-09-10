#!/usr/bin/env bash
# F-Droid's build, on this machine, in F-Droid's container.
#
# Every release so far met F-Droid's environment for the first time inside
# its CI -- forty minutes per attempt, a different failure each time. This
# script runs the SAME build F-Droid's CI runs, mirrored line by line from
# fdroiddata/.gitlab-ci.yml ("fdroid build" job), in the same image, with the
# same fdroidserver, on your machine, in minutes. The APK it produces is the
# APK F-Droid will produce for the same recipe commit. Sign that, attach it
# to the GitHub release, and CI's comparison passes instead of being a step.
#
#   ./tools/fdroid_build_local.sh 102             # versionCode from the recipe
#
# Requires: podman (or docker), and the fdroiddata clone with the recipe
# branch checked out (release_fdroid.sh step 4 does that; or do it by hand:
#   git -C $FDROIDDATA checkout com.clavierhaus.gnubg ).
# Output: $FDROIDDATA/tmp/com.clavierhaus.gnubg_<code>.apk  (unsigned)
# Cache:  a named volume keeps the NDK, gradle and SDK downloads between runs.

set -euo pipefail

APPID=com.clavierhaus.gnubg
FDROIDDATA="${FDROIDDATA:-$HOME/fdroiddata}"
IMAGE="registry.gitlab.com/fdroid/fdroidserver:buildserver-trixie"
CODE="${1:-}"

die() { printf '  x  %s\n' "$*" >&2; exit 1; }
ok()  { printf '  ok %s\n' "$*"; }

[ -n "$CODE" ] || die "usage: $0 <versionCode>   (e.g. 102)"
[ -d "$FDROIDDATA/.git" ] || die "fdroiddata clone not found at $FDROIDDATA (set FDROIDDATA=...)"
[ -f "$FDROIDDATA/metadata/$APPID.yml" ] || die "no metadata/$APPID.yml in $FDROIDDATA -- check out the recipe branch first"
grep -q "^    versionCode: $CODE\$" "$FDROIDDATA/metadata/$APPID.yml" \
  || die "recipe has no build block with versionCode $CODE"

if command -v podman >/dev/null; then RT=podman; elif command -v docker >/dev/null; then RT=docker; else die "podman or docker required"; fi

mkdir -p "$FDROIDDATA/tmp" "$FDROIDDATA/unsigned" "$FDROIDDATA/logs" "$FDROIDDATA/srclibs" "$FDROIDDATA/build"
rm -f "$FDROIDDATA/tmp/${APPID}_${CODE}.apk"

printf 'fdroiddata: %s (%s)\nimage:      %s\nbuild:      %s:%s\n' \
  "$FDROIDDATA" "$(git -C "$FDROIDDATA" rev-parse --short HEAD)" "$IMAGE" "$APPID" "$CODE"

# The container script below is fdroiddata's "fdroid build" job, with the
# GitLab-only parts removed (cache, artifacts, find-changed-builds -- we name
# the build ourselves). Nothing else is different, on purpose.
$RT run --rm -it \
  -v "$FDROIDDATA:/builds/fdroiddata:Z" \
  -v fdroid-build-cache-ndk:/opt/android-sdk/ndk:Z \
  -v fdroid-build-cache-gradle:/home/vagrant/.gradle:Z \
  -e CI_PROJECT_DIR=/builds/fdroiddata \
  -e BUILD="$APPID:$CODE" \
  "$IMAGE" bash -c '
set -euo pipefail
export home_vagrant=/home/vagrant
export fdroidserver=$home_vagrant/fdroidserver
export LC_ALL=C.UTF-8
cd $CI_PROJECT_DIR

# fdroidserver at master, exactly as CI installs it
rm -rf $fdroidserver && mkdir $fdroidserver
curl --silent https://gitlab.com/fdroid/fdroidserver/-/archive/master/fdroidserver-master.tar.gz \
  | tar -xz --directory=$fdroidserver --strip-components=1
export PATH="$fdroidserver:$PATH"
export PYTHONPATH="$fdroidserver:$fdroidserver/examples"
export PYTHONUNBUFFERED=true
export serverwebroot=/tmp
git -C $home_vagrant/gradlew-fdroid pull >/dev/null 2>&1 || true

for d in logs tmp unsigned $home_vagrant/.android $home_vagrant/.gradle $home_vagrant/metadata; do
  test -d $d || mkdir -p $d; chown -R vagrant $d; done
ln -sfn $home_vagrant/.gradle $CI_PROJECT_DIR/.gradle
ln -sfn $CI_PROJECT_DIR/tmp $home_vagrant/tmp
ln -sfn $CI_PROJECT_DIR/srclibs $home_vagrant/srclibs
sysctl fs.inotify.max_user_watches=524288 >/dev/null 2>&1 || true
export GRADLE_USER_HOME=$home_vagrant/.gradle
fdroid="sudo --preserve-env --user vagrant env PATH=$fdroidserver:$PATH env PYTHONPATH=$fdroidserver:$fdroidserver/examples env PYTHONUNBUFFERED=true env TERM=${TERM:-dumb} env HOME=$home_vagrant fdroid"

apt-get install -y sudo >/dev/null
apt-get install -y openjdk-21-jdk-headless >/dev/null
update-alternatives --set java /usr/lib/jvm/java-21-openjdk-amd64/bin/java
cp -R $CI_PROJECT_DIR/build $home_vagrant/build
appid=${BUILD%:*}
[ -d metadata/$appid ] && cp -R metadata/$appid $home_vagrant/metadata
cp -R metadata/$appid.yml $home_vagrant/metadata
chown -R vagrant $home_vagrant $CI_PROJECT_DIR /opt/android-sdk/ndk
pushd $home_vagrant >/dev/null
ln -sfn $CI_PROJECT_DIR $home_vagrant/fdroiddata
$fdroid fetchsrclibs $BUILD --verbose
rm -f $home_vagrant/fdroiddata
(unset CI; $fdroid build --verbose --test --refresh-scanner --on-server --no-tarball $BUILD)
popd >/dev/null
'

APK="$FDROIDDATA/tmp/${APPID}_${CODE}.apk"
[ -f "$APK" ] || die "no APK at $APK -- read the output above; this is the failure CI would have shown"
ok "F-Droid's build produced $APK"
sha256sum "$APK"
