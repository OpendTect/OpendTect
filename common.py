#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Bert
# DATE     : July 2018
#
# tools common to all odpy scripts
#

import platform
import sys

def dbg_msg( msg ):
  sys.stderr.write( msg + "\n" )

if platform.python_version() < "3":
  dbg_msg( "odpy requires at least pyhon 3" )
  exit( 1 )
