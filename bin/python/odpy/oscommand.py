#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : A. Huck
# DATE     : Apr 2019
#
# tools for managing OS commands
#

import sys
import os
import signal
import json
from subprocess import check_output, CalledProcessError

from odpy.common import isWin, getExecPlfDir, get_std_stream, std_msg

def getODCommand(execnm,args=None):
  cmd = list()
  cmd.append( os.path.join(getExecPlfDir(args),execnm) )
  return appendDtectArgs( cmd, args )

def appendDtectArgs( cmd, args=None ):
  if args == None:
    return cmd
  if 'dtectdata' in args:
    cmd.append( '--dataroot' )
    cmd.append( args['dtectdata'][0] )
  if 'survey' in args:
    cmd.append( '--survey' )
    cmd.append( args['survey'][0] )
  return cmd

def getPythonCommand(scriptfile,posargs=None,dict=None,args=None):
  cmd = list()
  if isWin():
    cmd.append( "python.exe" )
  else:
    cmd.append( "python3" )
  cmd.append( scriptfile )
  cmd = appendDtectArgs( cmd, args )
  cmd.append( '--dtectexec' )
  cmd.append( getExecPlfDir(args) )
  if args != None and 'proclog' in args:
    cmd.append( '--proclog' )
    cmd.append( args['proclog'] )
  if args != None and 'syslog' in args:
    cmd.append( '--syslog' )
    cmd.append( args['syslog'] )
  for posarg in posargs:
    cmd.append( posarg )
  if dict != None:
    cmd.append( '--dict' )
    cmd.append( json.dumps(dict) )
  return cmd

def execCommand( cmd, background=False ):
  if background:
    return startDetached( cmd )
  else:
    return startAndWait( cmd )

def killproc( pid=None ):
  if pid != None:
    os.kill( pid, signal.SIGTERM )
  return None

# INTERNAL, you should not need to use those:
def startAndWait( cmd ):
  try:
    procpid = check_output( cmd, stderr=get_std_stream() )
  except CalledProcessError as err:
    std_msg( 'Failed: ', err )
    raise FileNotFoundError
  return procpid

def startDetached( cmd ):
  return os.spawnvp( os.P_NOWAIT, cmd[0], cmd )
