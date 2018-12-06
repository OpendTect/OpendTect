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
import logging.config

LOGGING = {
  'version': 1,
  'handlers': {
    'console': {
      'class': 'logging.StreamHandler',
      'level': 'INFO',
      'stream': 'ext://sys.stdout'
    },
    'logfile': {
      'class': 'logging.StreamHandler',
      'level': 'DEBUG',
      'stream': 'ext://sys.stdout'
    },
    'errors': {
      'class': 'logging.StreamHandler',
      'level': 'WARNING',
      'stream': 'ext://sys.stdout'
    },
    'other': {
      'class': 'logging.StreamHandler',
      'level': 'CRITICAL',
      'stream': 'ext://sys.stderr'
    }
  },
  'loggers': {
    'dbg': {
      'level': 'WARNING',
      'handlers': ['errors'],
      'propagate': 'no'
    },
    'std': {
      'level': 'DEBUG',
      'handlers': ['console'],
      'propagate': 'no'
    },
    'log': {
      'level': 'DEBUG',
      'handlers': ['logfile'],
      'propagate': 'no'
    }
  },
  'root': {
    'level': 'CRITICAL',
    'handlers': ['other']
  }
}

def initLogging(args):
  set_log_file( args['logfile'].name )
  set_log_file( args['sysout'].name, 'console' )
  logging.config.dictConfig(LOGGING)

def set_log_file( filenm, handlernm='logfile' ):
  if filenm == 'sys.stdout':
    return
  if not os.path.exists(filenm):
    dbg_msg( 'Log file not found: ', filenm )
    return
  LOGGING['handlers'][handlernm] = {
    'class': 'logging.FileHandler',
    'filename': filenm,
    'mode': 'a'
  }

def get_log_logger():
  return logging.getLogger('log')

def get_std_logger():
  return logging.getLogger('std')

def dbg_msg(msg):
  logging.getLogger('dbg').warning(msg)

def std_msg(msg):
  get_std_logger().info(msg)

def log_msg(msg):
  get_log_logger().debug(msg)

def has_stdlog_file():
  return isinstance( get_std_logger().handlers[0], logging.FileHandler )

def has_log_file():
  return isinstance( get_log_logger().handlers[0], logging.FileHandler )

def get_stdlog_file():
  return get_std_logger().handlers[0].baseFilename

def get_log_file():
  return get_log_logger().handlers[0].baseFilename

if platform.python_version() < "3":
  dbg_msg( "odpy requires at least Python 3" )
  exit( 1 )

