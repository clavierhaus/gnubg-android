#!/usr/bin/env bash
# setup_signing.sh -- one-time, fire-and-forget release-tag signing.
#
# After you run this ONCE, `git tag -s` is automatic for every release:
# release.sh sees tag.gpgSign=true and signs each tag with no further action.
# Re-running is safe (idempotent): it reuses an existing key and re-asserts
# the config.
#
# What it does:
#   1. Finds or creates a signing key (GPG by default; --ssh uses an SSH key,
#      simpler and keyless-of-GPG if you already have one).
#   2. Sets git config (repo-local by default; --global to sign everywhere):
#      user.signingKey, tag.gpgSign=true, commit.gpgSign optional, and for
#      SSH the gpg.format=ssh + allowed-signers plumbing.
#   3. Prints the PUBLIC key and the exact `gh` command to register it on
#      GitHub so the web UI shows "Verified" on your tags.
#
# It NEVER uploads a private key anywhere and NEVER pushes; the only network
# step is the copy-pasteable GitHub registration you run yourself.
set -eu

SCOPE="--local"; MODE="gpg"; NAME="clavierhaus"; EMAIL="gnubg@clavierhaus.at"
while [ $# -gt 0 ]; do
  case "$1" in
    --global) SCOPE="--global" ;;
    --ssh) MODE="ssh" ;;
    --gpg) MODE="gpg" ;;
    --name) NAME="$2"; shift ;;
    --email) EMAIL="$2"; shift ;;
    *) echo "unknown arg: $1"; exit 2 ;;
  esac; shift
done
cd "$(git rev-parse --show-toplevel)"

say() { printf '\033[1m%s\033[0m\n' "$*"; }

if [ "$MODE" = "ssh" ]; then
  command -v ssh-keygen >/dev/null || { echo "ssh-keygen not found"; exit 1; }
  KEY="$HOME/.ssh/gnubg_signing"
  if [ ! -f "$KEY" ]; then
    say "creating an SSH signing key at $KEY"
    ssh-keygen -t ed25519 -C "$EMAIL (gnubg release signing)" -f "$KEY" -N ""
  else
    say "reusing existing SSH key $KEY"
  fi
  ALLOWED="$HOME/.ssh/gnubg_allowed_signers"
  printf '%s %s\n' "$EMAIL" "$(cat "$KEY.pub")" > "$ALLOWED"
  git config $SCOPE gpg.format ssh
  git config $SCOPE user.signingKey "$KEY.pub"
  git config $SCOPE gpg.ssh.allowedSignersFile "$ALLOWED"
  git config $SCOPE tag.gpgSign true
  PUB="$(cat "$KEY.pub")"
  say "DONE. git will sign every tag with your SSH key."
  echo
  say "Register it on GitHub as a SIGNING key (one time):"
  echo "  gh ssh-key add $KEY.pub --type signing --title 'gnubg release signing'"
  echo
  echo "Public key (if you prefer the web UI, Settings > SSH and GPG keys):"
  echo "  $PUB"
else
  command -v gpg >/dev/null || { echo "gpg not found -- install gnupg, or re-run with --ssh"; exit 1; }
  # Headless-safe: no interactive pinentry (we create a passphraseless key).
  export GPG_TTY="${GPG_TTY:-$(tty 2>/dev/null || true)}"
  GPGB="gpg --batch --pinentry-mode loopback"
  KEYID="$(gpg --list-secret-keys --with-colons "$EMAIL" 2>/dev/null \
            | awk -F: '/^sec:/ {print $5; exit}')"
  if [ -z "${KEYID:-}" ]; then
    say "creating a GPG signing key for $NAME <$EMAIL> (no passphrase, unattended)"
    $GPGB --passphrase '' --quick-generate-key "$NAME <$EMAIL>" ed25519 sign 0
    KEYID="$(gpg --list-secret-keys --with-colons "$EMAIL" \
              | awk -F: '/^sec:/ {print $5; exit}')"
  else
    say "reusing existing GPG key $KEYID for <$EMAIL>"
  fi
  git config $SCOPE user.signingKey "$KEYID"
  git config $SCOPE tag.gpgSign true
  say "DONE. git will sign every tag with GPG key $KEYID."
  echo
  say "Register it on GitHub so tags show 'Verified' (one time):"
  echo "  gpg --armor --export $KEYID | gh gpg-key add -"
  echo
  echo "  (or paste the block below into Settings > SSH and GPG keys > New GPG key)"
  gpg --armor --export "$KEYID"
fi

echo
say "Verify a signed tag any time with:  git tag -v <tag>"
