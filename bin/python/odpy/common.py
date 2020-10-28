"""Common tools for odpy package

Copyright (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  * AUTHOR : A. Huck
  * DATE   : July 2018

"""

import sys
import os
import platform
import logging
import tempfile
from datetime import datetime
import threading

def sTimeUnitString( ismilli=False, abbr=True ):
  """OpendTect-like time stamp

  Parameters:
    * ismilli (bool, optional): Include millisecond (default is False)
    * abbr (bool, optional): Abbreviated (default is True)

  Returns:
    * str: Time stamp string formatted like done by OpendTect

  Examples:
    >>> sTimeUnitString()
    'Mon 20 Apr 2020, 13:59:54'

    >>> sTimeUnitString( True )
    'Mon 20 Apr 2020, 13:59:54.001245'

    >>> sTimeUnitString( True, True )
    'Mon 20 Apr 2020, 13:59:54.001245'

    >>> sTimeUnitString( True, False )
    'Monday 20 April 2020, 13:59:54'

  """

  if abbr:
    fmt = "%a %d %b"
  else:
    fmt = "%A %d %B"
  fmt += " %Y, %X"
  if ismilli:
    fmt += ".%f"
  return datetime.now().strftime(fmt)

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
  """odpy Logger initialization

  Parameters:
    * args (dict):
      The members 'logfile' and 'sysout' from the input dictionary are supposed to contain existing filenames which are used
      to setup the module loggers proclog_logger and syslog_logger

  """

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
  """Print to odpy standard logger

  Parameters:
    * a (object or string): Message to be printed
    * b-f (object or string, optional): Message to be printed

  Returns:
    * str: Concatenated string

  Notes:
    * All objects are formatted to strings using the str() function
    * All outputs are automatically separated by spaces.
    * Reserved for standard logging information.

  """

  msg = mergeArgs(a,b,c,d,e,f)
  get_std_logger().info(msg)

def log_msg(a,b=None,c=None,d=None,e=None,f=None):
  """Print to odpy processing logger

  Parameters:
    * a (object or string): Message to be printed
    * b-f (object or string, optional): Message to be printed

  Returns:
    * str: Concatenated string

  Notes:
    * All objects are formatted to strings using the str() function
    * All outputs are automatically separated by spaces.
    * Reserved for processing logging information,
    * i.e. to report progress on a task

  """
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
    if isinstance( handler, logging.StreamHandler ) and handler.level != logging.NOTSET:
      return handler.stream
    elif isinstance( handler, logging.FileHandler ) and handler.level != logging.NOTSET:
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
  """Log file reset

  Parameters:
    * keeplines (int, optional): 
      Number of lines from the top of the file to keep (default is 0)

  Empty the log file pointed at by the processing logger,
  for instance before starting a new task.

  """

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
  """Stdout-stderr redirection

  * Forces stdout to point to odpy.proclog_logger
  * Forces stderr to point to odpy.syslog_logger

  Notes:
    * Changes the value of sys.stdout and sys.stderr
    * Should be avoided as much as possible: To be used only when one cannot
      redirect in any other way.

  """
  if (logconfig is  None) or (not logging.getLogger() == logconfig.root_logger):
    return
  if has_log_file():
    sys.stdout = open( get_log_file(), 'a' )
  if has_stdlog_file():
    sys.stderr = open( get_stdlog_file(), 'a' )

def restore_stdout():
  """Stdout-stderr restore

  Undo operation of redirect_stdout

  """

  if (logconfig is None) or (not logging.getLogger() == logconfig.root_logger):
    return
  if has_log_file():
    sys.stdout = sys.__stdout__
  if has_stdlog_file():
    sys.stderr = sys.__stderr__

def isWin():
  """Is platform Windows?

  Returns:
    * True if running on any Windows platform

  """

  return platform.system() == 'Windows'

def isMac():
  """Is platform Mac?

  Returns:
    * True if running on any Mac-OS platform

  """

  return platform.system() == 'Darwin'

if platform.python_version() < "3":
  std_msg( "odpy requires at least Python 3" )
  sys.exit( 1 )

def getPlfSubDir():
  """Platform sub-directory

  Platform specific sub-directory as existing
  in an OpendTect software installation
  
  Returns:
    * str: string like: 'lux64', 'win64', 'mac'.
      None if the platform is not supported by OpendTect

  """

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
  """Binary sub-directory

  Sub-directory containing the executables in an OpendTect installation

  Returns:
    * str: string like: 'Debug', 'Release', 'RelWithDebInfo'. 
      None if no such sub-directory is found.

  Notes:
    Assumes that the OpendTect installation contains the 
    executable 'od_FileBrowser', and that this installation can
    be located by the function getODSoftwareDir()

  """

  if isMac():
    return None
  plfsubdirfp = os.path.join( getODSoftwareDir(), 'bin', getPlfSubDir() )
  execnm = 'od_FileBrowser'
  if isWin():
    execnm = execnm+'.exe'
  reltypes = ('Debug','Release','RelWithDebInfo')
  for reltype in reltypes:
    if os.path.isfile( os.path.join(plfsubdirfp,reltype,execnm) ):
      return reltype
  return None

def getODSoftwareDir(args=None):
  """OpendTect sofware directory

  Parameters:
    * args (dict, optional):
      Dictionary with the member 'dtectexec'. The value
      for that member should point to the executables folder
      of the requested application

  Returns:
    * str: Full path to the OpendTect software installation

  Notes:
    * Retrieved from either the input dictionary
      or from the current environment by reading
      the 'DTECT_APPL' or 'DTECT_WINAPPL' variables which
      are set by OpendTect at runtime.
    * Neither dictionary nor environment variables need to be set
      if the current module is placed within an OpendTect installation

  Examples:
    >>> getODSoftwareDir()
    'C:\\Program Files\\OpendTect\\6.6.0'

  """

  if args != None and 'dtectexec' in args:
    bindir = getExecPlfDir(args)
    return bindir
  applenvvar = 'DTECT_APPL'
  if isWin():
    applenvvar = 'DTECT_WINAPPL'
  if applenvvar in os.environ:
    return os.environ[applenvvar]
  curdir = os.path.dirname( __file__ )
  maxrecur = 15
  relinfodir = 'relinfo'
  if isMac():
    relinfodir = os.path.join('Resources',relinfodir)
  while not os.path.isdir(os.path.join(curdir,relinfodir)) and maxrecur > 0:
    curdir = os.path.dirname( curdir )
    maxrecur = maxrecur-1
  return curdir

def getExecPlfDir(args=None):
  """OpendTect executables directory

  Parameters:
    * args (dict, optional):
      Dictionary with the member 'dtectexec'. The value
      for that member should point to the executables folder
      of the requested application

  Returns:
    * str: Full path to the binaries of an OpendTect installation

  Notes:
    * Assumes that the software installation can be located with getODSoftwareDir()
      and that python is running on a platform supported by OpendTect.

  Examples:
    >>> getExecPlfDir()
    'C:\\Program Files\\OpendTect\\6.6.0\\bin\\win64\\Release'

  """

  if args != None and 'dtectexec' in args:
    return args['dtectexec'][0]
  appldir = getODSoftwareDir()
  if isMac():
    return os.path.join( getODSoftwareDir(), 'Contents', 'MacOS' )
  else:
    return os.path.join( getODSoftwareDir(), 'bin', getPlfSubDir(), getBinSubDir())

def getODArgs(args=None):
  """OpendTect arguments dictionary

  Create a dictionary that contains typical OpendTect
  command line arguments and the files from this module' loggers

  Parameters:
    * args (dict, optional):
      Dictionary with the member 'dtectexec'. The value
      for that member should point to the executables folder
      of the requested application

  Returns:
    * dict: A dictionary with the following key-values:
        * 'dtectexec' : Full path to the OpendTect installation (see getExecPlfDir)
        * 'survey' : The survey directory name, if provided by the input dictionary
        * 'proclog' : The log file from proclog_logger if applicable
        * 'syslog' : The log file frol syslog_logger if applicable

  """

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
#  else:
#    if has_log_file():
#      ret.update({'syslog': args['logfile'].name})
  return ret

def getIconFp(nm,args=None):
  """Path to an OpendTect icon file

  Parameters:
    * nm (string): Icon file basename (without extension)
    * args (dict, optional)
      Dictionary with the member 'dtectexec'. The value
      for that member should point to the executables folder
      of the requested application

  Returns:
    * str: Full path to the icon folder of the OpendTect installation

  Notes:
    Assumes that the software installation can be located with getODSoftwareDir()

  Examples:
    >>> getIconFp( 'dgbpro' )
    'C:\\Program Files\\OpendTect\\6.6.0\\data\\icons.Default\\dgbpro.png'
    
  """
  oddir = getODSoftwareDir(args)
  ret = os.path.join(oddir,'data','icons.Default',nm)+'.png'
  if os.path.exists(ret):
    return ret
  return None

def tail(fp,lines=1,strip_empty=False,_buffer=4098):
  lines_found = []
  block_counter = -1
  while len(lines_found) < lines:
    try:
      fp.seek(block_counter * _buffer, os.SEEK_END)
    except IOError:
      fp.seek(0)
      lines_found = fp.readlines()
      break

    lines_found = fp.readlines()
    block_counter -= 1

  ret = lines_found[-lines:]
  if strip_empty:
    while '\n' in ret:
      ret.remove('\n')
  return ret

def batchIsFinished( logfile ):
  """OpendTect batch processing status

  Checks if an OpendTect batch process reached completion by
  parsing its log file.

  Parameters:
    logfile (str): Full path to an existing OpendTect log file.

  Returns:
    * bool:
      True if the file exists and contains 'Finished batch processing' at its tail.

  Notes:
    Instantaneous status. To monitor if the processing finishes,
    the user must repeatedly call this function.

  """

  ret = list()
  with open(logfile) as fd:
    ret = tail(fd,10,True)
  return len(ret) > 0 and 'Finished batch processing' in ret[-1]

def writeFile( fnm, content ):
  try:
    f = open( fnm, 'w' )
    f.write( content )
    f.close()
  except:
    return False
  return True

def getTempDir():
    # This HAS to correspond with the C++ getTempDir() function
    return tempfile.gettempdir()

class Timer(threading.Timer):
    """Repeated timer

    Timer that restarts automatically after each interval
    """

    def run(self):
        while not self.finished.is_set():
            self.finished.wait(self.interval)
            self.function(*self.args, **self.kwargs)
        self.finished.set()
