#!/bin/sh

# Grats to Warp Rogue where I grabbed this process from.
# All errors are mine.


# Setup local variables
TARGET=powder
VERSION=`cat ../../VERSION.TXT`
GAME_NAME=POWDER_$VERSION

echo $GAME_NAME

# Magic sed string to adjust our values
INFO_PLIST_PATTERN="s/\$(APP_NAME)/$GAME_NAME/;s/\$(EXEC_NAME)/$TARGET/;s/\$(VERSION)/$VERSION/;s+\$(ICON_NAME)+icon_mac.icns+;s/\$(SIGNATURE)/POWD/;s/\$(IDENT)/com.zincland.powder/"

echo Populating app directory
rm -rf app
mkdir app
cp -R Template.app "app/$GAME_NAME.app"
sed $INFO_PLIST_PATTERN Template.app/Contents/Info.plist > "app/$GAME_NAME.app/Contents/Info.plist"
cp $TARGET "app/$GAME_NAME.app/Contents/MacOS/$TARGET"
ln -s "Contents/MacOS/$TARGET" "app/$GAME_NAME.app/$TARGET"

# copy in our license, etc.
cp ../../LICENSE.TXT "app"
cp ../../README.TXT "app"
cp ../../CREDITS.TXT "app"

# Ensure user doesn't need to install SDL.
cp -R "/Library/Frameworks/SDL.framework" "app/$GAME_NAME.app/Contents/Frameworks"

# Clean up unnecessary SDL files
rm -r "app/$GAME_NAME.app/Contents/Frameworks/SDL.framework/Versions/A/Headers"
rm "app/$GAME_NAME.app/Contents/Frameworks/SDL.framework/Headers"

# TODO: Copy an icon to Contents/Resources:w
cp ../../gfx/icon_mac.icns "app/$GAME_NAME.app/Contents/Resources"

echo Building a DMG

rm -f "$TARGET.dmg"

hdiutil create -srcfolder "app" -fs HFS+ -volname "$GAME_NAME" "$TARGET.tmp.dmg"
hdiutil convert -format UDZO -imagekey zlib-level=9 "$TARGET.tmp.dmg" -o "$TARGET.dmg"
hdiutil internet-enable -yes "$TARGET.dmg"
rm -f "$TARGET.tmp.dmg"

echo Installing to home
cp $TARGET.dmg ~
