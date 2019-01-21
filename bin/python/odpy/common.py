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
import logging

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

def has_stdlog_file():
  return isinstance( get_std_logger().handlers[0], logging.FileHandler )

def has_log_file():
  return isinstance( get_log_logger().handlers[0], logging.FileHandler )

def get_stdlog_file():
  return get_std_logger().handlers[0].baseFilename

def get_log_file():
  return get_log_logger().handlers[0].baseFilename

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
  if (logconfig is None) or (not logging.getLogger() == logconfig.root_logger):
    return
  if has_log_file():
    sys.stdout = open( get_log_file(), 'a' )
  if has_stdlog_file():
    sys.stderr = open( get_stdlog_file(), 'a' )

def restore_stdout():
  if (logconfig is None) or (not logging.getLogger() == logconfig.root_logger):
    return
  if has_log_file():
    sys.stdout = open( '<stdout>', 'w' )
  if has_stdlog_file():
    sys.stderr = open( '<stderr>', 'w' )

if platform.python_version() < "3":
  std_msg( "odpy requires at least Python 3" )
  sys.exit( 1 )

def getODCommand(args,execnm):
  cmd = list()
  cmd.append( os.path.join(args['dtectexec'][0],execnm) )
  if 'dtectdata' in args:
    cmd.append( '--dataroot' )
    cmd.append( args['dtectdata'][0] )
  if 'survey' in args:
    cmd.append( '--survey' )
    cmd.append( args['survey'][0] )
  return cmd
