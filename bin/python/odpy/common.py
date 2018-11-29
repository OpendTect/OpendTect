#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Bert
# DATE     : July 2018
#
# tools common to all odpy scripts
#

import os
import platform
import sys

class LogManager(object):
  def __init__(self):
    self.log_file = sys.stdout
    self.dbg_file = sys.stdout

  def set_log_file( self, fnm ):
    if not fnm:
      self.log_file = sys.stderr
    else:
      if os.path.isfile( fnm ):
        newlogfile = open( fnm, "a" )
      else:
        newlogfile = open( fnm, "w" )
      if newlogfile.closed:
        dbg_msg( "Could not open '" + fnm + "'" )
      else:
        self.log_file = newlogfile
        self.std_msg( "Log file set to '" + fnm + "'" )

  def std_msg( self, msg ):
    print( msg, file=sys.stdout )

  def log_msg( self, msg ):
    print( msg, file=self.log_file )

  def dbg_msg( self, msg ):
    print( msg, file=self.dbg_file )

  def err_msg( self, msg ):
    print( msg, file=sys.stderr )


if platform.python_version() < "3":
  dbg_msg( "odpy requires at least Python 3" )
  exit( 1 )

