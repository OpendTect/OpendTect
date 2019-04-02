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
from time import sleep
from subprocess import check_output, run, CalledProcessError

from odpy.common import isWin, getExecPlfDir, log_msg, std_msg

def getODCommand(execnm,args=None):
  cmd = list()
  cmd.append( path.join(getExecPlfDir(args),execnm) )
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

def execCommand( cmd, background=False, stdout=None, stderr=None, args=None ):
  if background:
    return startDetached( cmd, stdout, stderr )
  else:
    return startAndWait( cmd, args )

def killproc( pid=None ):
  if pid == None:
    return
  std_msg( 'Killing the process with PID', pid )
  os.killpg( os.getpgid(pid), signal.SIGTERM )

# INTERNAL, you should not need to use those:
def startAndWait( cmd, args=None ):
  stderrstrm = sys.stderr
  if args != None and 'logfile' in args:
    stderrstrm = args['logfile']
  ret = None
  try:
    ret = check_output( cmd, stderr=stderrstrm )
  except CalledProcessError as e:
    log_msg( 'Failed: ', e )
    raise FileNotFoundError
  return ret

def startDetached( cmd, stdout=None, stderr=None ):
  procpid = launchDetached( cmd, stdout, stderr )
  sleep( 0.1 )
  return procpid

def launchDetached( cmd, stdout, stderr ):
  try:
    pid = os.fork()
    if pid > 0:
      return pid
  except OSError as err:
    std_msg( 'fork', err.errno, 'failed:', err.strerror )
    sys.exit(1)
  os.setsid()
  return doDetached( cmd, stdout, stderr )

def doDetached( cmd, stdout, stderr ):
  forkpid = os.getpid()
  forkpgid = os.getpgid(forkpid)
  try:
    ret = run( cmd, check=True ) #TODO: add stdout, stderr
  except CalledProcessError as err:
    std_msg( err )
    os.setpgid( ret.pid, forkpgid )
  return forkpid
  
