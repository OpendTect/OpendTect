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
  def __init__(self, argv):
    self.log_file = sys.stdout
    self.std_file = sys.stdout
    self.dbg_file = sys.stdout
    self.err_file = sys.stderr
    if len(argv)>1 and not 'htm' in argv[1]:
      stdoutfnm = argv[1]
      self.set_stdout( stdoutfnm )
    if len(argv)>2:
      logfnm = argv[2]
      self.set_log_file( logfnm )

  def set_log_file( self, fnm ):
    if not fnm:
      self.log_file = sys.stderr
    else:
      if os.path.isfile( fnm ):
        newlogfile = open( fnm, "a" )
        self.std_msg( "Log file '" + fnm + "' opened in append mode" )
      else:
        newlogfile = open( fnm, "w" )
        self.std_msg( "Log file opened in write mode" )
      if newlogfile.closed:
        self.err_msg( "Could not open '" + fnm + "'" )
      else:
        self.log_file = newlogfile
        self.std_msg( "Log file set to '" + fnm + "'" )

  def set_stdout( self, fnm ):
    if fnm == "null":
      return
    if os.path.isfile( fnm ):
      newlogfile = open( fnm, "a" )
    else:
      newlogfile = open( fnm, "w" )
    if not newlogfile.closed:
      self.std_file = newlogfile

  def std_msg( self, msg ):
    print( msg, file=self.std_file )

  def log_msg( self, msg ):
    print( msg, file=self.log_file )

  def dbg_msg( self, msg ):
    print( msg, file=self.dbg_file )

  def err_msg( self, msg ):
    print( msg, file=self.err_file )


if platform.python_version() < "3":
  dbg_msg( "odpy requires at least Python 3" )
  exit( 1 )

