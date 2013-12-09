#!/bin/csh -f
#
# Checks missing icons in Default and Classic sets
# $Id$

cd $WORK/data/icons.Default
echo ""; echo "Icons in Default, missing in Classic:"
foreach fil ( *.png )
	if ( ! -e ../icons.Classic/$fil ) echo $fil
end

cd ../icons.Classic
echo ""; echo "Icons in Classic, missing in Default:"
foreach fil ( *.png )
	if ( ! -e ../icons.Default/$fil ) echo $fil
end
echo ""
