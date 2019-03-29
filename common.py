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
import json
import logging
from subprocess import check_output, run, CalledProcessError

try:
  from bokeh.util import logconfig
except ImportError:
  logconfig = None

syslog_logger = logging.getLogger(__name__)
proclog_logger = logging.getLogger('odproclog')

if not syslog_logger.hasHandlers():
  handler = logging.StreamHandler(sys.stdout)
  syslog_logger.setLevel( 'INFO' )
  syslog_logger.addHandler( handler )

if not proclog_logger.hasHandlers():
  handler = logging.StreamHandler(sys.stdout)
  proclog_logger.setLevel( 'DEBUG' )
  proclog_logger.addHandler( handler )

def initLogging(args):
  set_log_file( args['logfile'].name, proclog_logger )
  set_log_file( args['sysout'].name, syslog_logger )

def set_log_file( filenm, logger ):
  for handler in logger.handlers:
    logger.removeHandler( handler )
  if filenm == '<stdout>':
    handler = logging.StreamHandler(sys.stdout)
    logger.addHandler( handler )
    return
  elif filenm == '<stderr>':
    handler = logging.StreamHandler(sys.stderr)
    logger.addHandler( handler )
    return
  if not os.path.exists(filenm):
    std_msg( 'Log file not found: ', filenm )
    return
  handler = logging.FileHandler(filenm,'a')
  logger.addHandler( handler )
  logger.propagate = False

def get_log_logger():
  return proclog_logger

def get_std_logger():
  return syslog_logger

def mergeArgs(a,b=None,c=None,d=None,e=None,f=None):
  msg = str(a)
  if b != None:
    msg = msg+' '+str(b)
  if c != None:
    msg = msg+' '+str(c)
  if d != None:
    msg = msg+' '+str(d)
  if e != None:
    msg = msg+' '+str(e)
  if f != None:
    msg = msg+' '+str(f)
  return msg

def std_msg(a,b=None,c=None,d=None,e=None,f=None):
  msg = mergeArgs(a,b,c,d,e,f)
  get_std_logger().info(msg)

def log_msg(a,b=None,c=None,d=None,e=None,f=None):
  msg = mergeArgs(a,b,c,d,e,f)
  get_log_logger().debug(msg)

def has_file_handlers(logger):
  for handler in logger.handlers:
    if isinstance( handler, logging.FileHandler ):
      return True
  return False

def has_stdlog_file():
  return has_file_handlers( get_std_logger() )

def has_log_file():
  return has_file_handlers( get_log_logger() )

def get_handler_stream(logger):
  for handler in logger.handlers:
    if isinstance( handler, logging.StreamHandler ):
      return handler.stream
    elif isinstance( handler, logging.FileHandler ):
      return handler.stream
  return None

def get_handler_filename(logger):
  for handler in logger.handlers:
    if isinstance( handler, logging.FileHandler ):
      return handler.baseFilename
  return None

def get_std_stream():
  return get_handler_stream( get_std_logger() )

def get_log_stream():
  return get_handler_stream( get_log_logger() )

def get_stdlog_file():
  return get_handler_filename( get_std_logger() )

def get_log_file():
  return get_handler_filename( get_log_logger() )

def reset_log_file( keeplines=0 ):
  if not has_log_file():
    return
  logfnm = get_log_file()
  idx = 0
  f = open( logfnm, 'r' )
  keptlines = list()
  for line in f:
    keptlines.append( line )
    idx = idx + 1
    if idx >= keeplines:
      break
  f.close()
  f = open( logfnm, 'w' )
  for line in keptlines:
    f.write( line )
  f.close()
  set_log_file( logfnm, proclog_logger )

def redirect_stdout():
  if (logconfig is  None) or (not logging.getLogger() == logconfig.root_logger):
    return
  if has_log_file():
    sys.stdout = open( get_log_file(), 'a' )
  if has_stdlog_file():
    sys.stderr = open( get_stdlog_file(), 'a' )

def restore_stdout():
  if (logconfig is None) or (not logging.getLogger() == logconfig.root_logger):
    return
  if has_log_file():
    sys.stdout = sys.__stdout__
  if has_stdlog_file():
    sys.stderr = sys.__stderr__

if platform.python_version() < "3":
  std_msg( "odpy requires at least Python 3" )
  sys.exit( 1 )

def getPlfSubDir():
  system = platform.system()
  arch = platform.architecture()[0]
  if system == 'Linux':
    if arch == '64bit':
      return 'lux64'
    else:
      return None
  elif system == 'Windows':
    if arch == '64bit':
      return 'win64'
    else:
      return 'win32'
  elif system == 'Darwin':
    if arch == '64bit':
      return 'mac'
    else:
      return None
  else:
    return None

def getBinSubDir():
  if platform.system() == 'Darwin':
    return None
  plfsubdirfp = os.path.join( getODSoftwareDir(), 'bin', getPlfSubDir() )
  execnm = 'od_main'
  if platform.system() == 'Windows':
    execnm = execnm+'.exe'
  reltypes = ('Debug','Release','RelWithDebInfo')
  for reltype in reltypes:
    if os.path.isfile( os.path.join(plfsubdirfp,reltype,execnm) ):
      return reltype
  return None

def getODSoftwareDir(args=None):
  if args != None and 'dtectexec' in args:
    bindir = getExecPlfDir(args)
    return bindir
  applenvvar = 'DTECT_APPL'
  if platform.system() == 'Windows':
    applenvvar = 'DTECT_WINAPPL'
  if applenvvar in os.environ:
    return os.environ[applenvvar]
  curdir = os.path.dirname( __file__ )
  maxrecur = 15
  relinfodir = 'relinfo'
  if platform.system() == 'Darwin':
    relinfodir = os.path.join('Resources',relinfodir)
  while not os.path.isdir(os.path.join(curdir,relinfodir)) and maxrecur > 0:
    curdir = os.path.dirname( curdir )
    maxrecur = maxrecur-1
  return curdir

def getExecPlfDir(args=None):
  if args != None and 'dtectexec' in args:
    return args['dtectexec'][0]
  appldir = getODSoftwareDir()
  if platform.system() == 'Darwin':
    return os.path.join( getODSoftwareDir(), 'Contents', 'MacOS' )
  else:
    return os.path.join( getODSoftwareDir(), 'bin', getPlfSubDir(), getBinSubDir())

def getODArgs(args=None):
  ret = {
    'dtectexec': [getExecPlfDir(args)]
  }
  if args != None and 'dtectdata' in args:
    ret.update({'dtectdata': args['dtectdata']})
  if args != None and 'survey' in args:
    ret.update({'survey': args['survey']})
  if has_log_file() :
    ret.update({'proclog': args['logfile'].name})
  if has_stdlog_file():
    ret.update({'syslog': args['sysout'].name})
  return ret

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

def getODCommand(execnm,args=None):
  cmd = list()
  cmd.append( os.path.join(getExecPlfDir(args),execnm) )
  return appendDtectArgs( cmd, args )

def getPythonCommand(scriptfile,posargs=None,dict=None,args=None):
  cmd = list()
  if platform.system() == 'Windows':
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

def runCommand( cmd, args=None ):
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
