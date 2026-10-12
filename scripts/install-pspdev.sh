#!/usr/bin/env bash
set -euo pipefail

if [[ $(uname -s) != Linux || $(uname -m) != x86_64 ]]; then
    echo 'This installer targets Linux x86_64. See PSPDEV releases for other platforms.' >&2
    exit 1
fi

tools_root="${PSP_TOOLS_ROOT:-/workspace/.tools}"
release=v20261001
expected_sha=39b20efc7c8492cdbd9929178f45afc801dabb0967b7c882573520f22536920c
install_root="$tools_root/pspdev-$release"
archive="$tools_root/downloads/pspdev-$release-debian.tar.gz"

mkdir -p "$tools_root/downloads"
if [[ -d "$install_root" ]]; then
    if [[ ! -f "$install_root/.archive-sha256" ]] || [[ $(cat "$install_root/.archive-sha256") != "$expected_sha" ]]; then
        echo "Refusing to overwrite an unrecognized installation: $install_root" >&2
        exit 1
    fi
else
    if [[ ! -f "$archive" ]]; then
        curl --fail --location --retry 2 --connect-timeout 20 --max-time 240 \
            "https://github.com/pspdev/pspdev/releases/download/$release/pspdev-debian-latest.tar.gz" \
            --output "$archive.part"
        printf '%s  %s\n' "$expected_sha" "$archive.part" | sha256sum --check --status
        mv "$archive.part" "$archive"
    fi
    printf '%s  %s\n' "$expected_sha" "$archive" | sha256sum --check
    staging=$(mktemp -d "$tools_root/pspdev-stage.XXXXXX")
    trap 'rm -rf "$staging"' EXIT
    tar --extract --gzip --file "$archive" --directory "$staging" --no-same-owner
    test -x "$staging/pspdev/bin/psp-gcc"
    test -f "$staging/pspdev/psp/sdk/lib/build.mak"
    printf '%s\n' "$expected_sha" > "$staging/.archive-sha256"
    mv "$staging" "$install_root"
    trap - EXIT
fi

export PSPDEV="$install_root/pspdev"
export PATH="$PSPDEV/bin:$PATH"
psp-gcc --version | head -n 1
psp-config --pspsdk-path
echo "PSPDEV installed at $PSPDEV"
