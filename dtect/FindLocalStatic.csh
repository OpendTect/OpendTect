#!/bin/csh 
#
# Copyright (C): dGB Beheer B. V.
#

set SCRIPTDIR=`dirname $0`

"${SCRIPTDIR}/FindKeyword.csh" --keyword '\)\s*{[^}]*\s+static\s+' $*
