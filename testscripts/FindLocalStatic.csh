#!/bin/csh 
#
# Copyright (C): dGB Beheer B. V.
#

set SCRIPTDIR=`dirname $0`

"${SCRIPTDIR}/FindKeyword.csh" --message "Local static variables are not allowed. Use mDefineStaticLocalObject macro instead." --grepcommand egrep --keyword '\)\s*{[^}]*\s+static\s+' $*
