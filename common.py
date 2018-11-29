#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Bert
# DATE     : July 2018
#
# tools common to all odpy scripts
#

import platform
import sys

log_file = sys.stdout
dbg_file = sys.stderr


def dbg_msg( msg ):
  dbg_file.write( msg + "\n" )

def set_log_file( fnm ):
  if not fnm:
    log_file = sys.stderr
  else:
    if os.path.isfile( fnm ):
      newlogfile = open( fnm, "a" )
    else:
      newlogfile = open( fnm, "w" )
    if newlogfile.closed:
      dbg_msg( "Could not open '" + fnm + "'" )
    else:
      log_file = newlogfile
      dbg_msg( "Log file set to '" + fnm + "'" )

def log_msg( msg ):
  log_file.write( msg + "\n" )
  

if platform.python_version() < "3":
  dbg_msg( "odpy requires at least Python 3" )
  exit( 1 )
