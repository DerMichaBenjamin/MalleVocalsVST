#!/bin/bash
set -euo pipefail
# Local keychain setup only. Never put certificate passwords or keys in source.
: "${SIGN_IDENTITY:?Set Developer ID Application identity from your local keychain}"
: "${NOTARY_PROFILE:?Set stored notarytool keychain profile}"
artifact_dir="${1:?Provide artifact directory}"
while IFS= read -r -d '' bundle; do
 codesign --force --deep --options runtime --timestamp --sign "$SIGN_IDENTITY" "$bundle"
 codesign --verify --deep --strict "$bundle"
done < <(find "$artifact_dir" -type d \( -name '*.vst3' -o -name '*.component' -o -name '*.app' \) -prune -print0)
ditto -c -k --sequesterRsrc --keepParent "$artifact_dir" "$artifact_dir-notarize.zip"
xcrun notarytool submit "$artifact_dir-notarize.zip" --keychain-profile "$NOTARY_PROFILE" --wait
while IFS= read -r -d '' bundle; do xcrun stapler staple "$bundle"; done < <(find "$artifact_dir" -type d \( -name '*.vst3' -o -name '*.component' -o -name '*.app' \) -prune -print0)
