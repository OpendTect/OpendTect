#!/bin/csh 
#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

set SCRIPTDIR=`dirname $0`

"${SCRIPTDIR}/FindKeyword.csh" --message "Local static variables are not allowed. Use mDefineStaticLocalObject macro instead." --grepcommand egrep --keyword '\)\s*{[^}]*\s+static\s+' $*
