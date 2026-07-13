# Release signing

Release tags are **signed**, and setup is **fire-and-forget**: run one script
once, and every future `./release.sh` signs its tag automatically with no
extra flags or steps. This gives a release *authenticity* (who cut it), on top
of the SHA256 checksum's *integrity* (the APK arrived intact).

## One-time setup

```
tools/setup_signing.sh            # GPG, repo-local (recommended)
```

Options: `--ssh` uses an SSH signing key instead of GPG (simplest if you
already have one); `--global` signs in every repo, not just this one;
`--name` / `--email` override the identity (defaults: clavierhaus /
gnubg@clavierhaus.at). Re-running is safe — it reuses an existing key.

The script creates (or reuses) a signing key, sets `tag.gpgSign=true` and
`user.signingKey`, and prints the **one** GitHub registration command to run
so the web UI shows *Verified* on your tags, e.g.:

```
gpg --armor --export <KEYID> | gh gpg-key add -     # GPG
gh ssh-key add ~/.ssh/gnubg_signing.pub --type signing --title 'gnubg release signing'   # SSH
```

It never uploads a private key and never pushes; that registration line is
the only network step, and you run it.

## What happens at release time

`release.sh` checks `git config tag.gpgSign`. If signing is set up it runs
`git tag -s` (signed); if not, it falls back to a plain annotated tag and
warns — a release is never *blocked* by missing signing, but the warning
tells you to run the setup once.

## Verifying a release

Integrity (anyone):
```
sha256sum -c app-debug.apk.sha256      # in the download directory
```
Authenticity (with the public key registered / imported):
```
git tag -v <tag>                       # -> "Good signature"
```

## Threat model, honestly

- The **checksum** proves the APK you downloaded matches what the release
  publishes — it catches corruption and truncation, not a hostile swap
  (an attacker who replaces the APK can replace its `.sha256` too).
- The **signed tag** binds the release commit to your key — that is the
  authenticity layer. Signing the APK *binary* directly (detached GPG or
  minisign over the .apk) would extend authenticity to the artifact itself;
  that is the natural next tier if wanted, and not yet done.
