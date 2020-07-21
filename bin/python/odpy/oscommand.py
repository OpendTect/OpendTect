"""Tools for managing OS commands

Copyright (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  * AUTHOR : A. Huck
  * DATE   : Apr 2019

"""

import sys
import os
import psutil
import signal
import subprocess
import json
import threading

from odpy.common import isWin, getExecPlfDir, get_log_stream, get_std_stream, std_msg, sTimeUnitString, log_msg

def getODCommand(execnm,args=None):
  """OpendTect command

  For a given OpendTect executable, get its full command line

  Parameters:
    * execnm (list): [Executable name]
    * arg (dict, optional):
      Dictionary with the members 'dtectdata' and 'survey' as 
      single element lists, and/or 'dtectexec' (see odpy.getODSoftwareDir)

  Returns:
    * list: OpendTect command line

  Notes:
    The two standard arguments providing the current survey location
    will be added to the command line arguments.

  Examples:
    >>> args = {
      'dtectdata': ['D:\\ODData'],
      'survey': ['F3_Demo']
    }

    >>> getODCommand()
    ['C:\\Program Files\\OpendTect\\6.6.0\\bin\win64\Release\\od_process_attrib',
    '--dataroot',
    'D:\\ODData',
    '--survey',
    'F3_Demo']

  """

  cmd = list()
  cmd.append( os.path.join(getExecPlfDir(args),execnm) )
  return appendDtectArgs( cmd, args )

def appendDtectArgs( cmd, args=None ):
  """Append OpendTect arguments

  Parameters:
    * cmd (list): List to which the returned elements will be added
    * arg (dict, optional):
      Dictionary with the members 'dtectdata' and 'survey' as 
      single element lists, and/or 'dtectexec' (see odpy.getODSoftwareDir)

  Returns:
    * list: Input list with the added data root and survey directory name if provided

  Examples:
    >>> args = {
      'dtectdata': ['D:\\ODData'],
      'survey': ['F3_Demo']
    }

    >>> appendDtectArgs( args )
    ['--dataroot', 'D:\\ODData', '--survey', 'F3_Demo']

  """

  if args == None:
    return cmd
  if 'dtectdata' in args:
    cmd.append( '--dataroot' )
    cmd.append( args['dtectdata'][0] )
  if 'survey' in args:
    cmd.append( '--survey' )
    cmd.append( args['survey'][0] )
  return cmd

def getPythonExecNm():
  """Python executable name

  shortcut to sys.executable
  
  """

  return sys.executable

def getPythonCommand(scriptfile,posargs=None,dict=None,args=None):
  """Python command line

  Parameters:
    * scriptfile (str): Python script or executable
    * posargs (list, optional): 
      Positional arguments for that script (default is None)
    * dict (str, optional): Value for the argument with key "--dict":
      String from which a json object can be parsed (default is None)
    * args (dict):
      Dictionary with the members 'dtectdata' and 'survey' as 
      single element lists, and/or 'dtectexec' (see odpy.getODSoftwareDir).
      The dictionary may also contain the members 'proclog' and 'syslog' as 
      strings, each being the full path to a log file.

  Returns:
    * list: 
      Fully formed command line to be executed by the python executable

  """

  cmd = list()
  cmd.append( getPythonExecNm() )
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
  """Command execution

  Launches the execution of a command with its arguments.

  Parameters:
    * cmd (list): command to be executed with its arguments.
    * background (bool, optional):
      The command is forked if True (default is False).

  Returns:
    * str: 
      The stdout print of the subprocess.CompletedProcess (default)
      or a psutil.Popen object (background=True)

  Notes:
    This is an interface to the subprocess module.

  """

  if background:
    return startDetached( cmd )
  else:
    return startAndWait( cmd )

def isRunning( proc ):
  """is the process running?

  Parameters:
    * proc (psutil.Popen): Process to be checked

  Returns:
    * bool: True if the process is running.

  """

  if proc == None:
    return
  proc.poll()
  return proc.is_running()

def pauseProcess( proc ):
  """Pause a running process

  Parameters:
    * proc (psutil.Popen): Process to be paused

  Returns:
    * Set and return subprocess.Popen.returncode

  """

  if isinstance(proc,psutil.Popen):
    return proc.suspend()

def resumeProcess( proc ):
  """Resume a paused process

  Parameters:
    * proc (psutil.Popen): Process to be resumed

  Returns:
    * Set and return subprocess.Popen.returncode

  """

  if isinstance(proc,psutil.Popen):
    return proc.resume()

def printProcessTime(procnm,isstart,print_fn=std_msg,withprocline=True):
  """Print processing timestamp

  Parameters:
    * procnm (str): Process name
    * isstart (bool): Status: Start or end of this process.
    * print_fn (function, optional):
      function to be used for the printing of the timestamp
      (default is odpy.std_msg, which default to stderr)
    * withprocline (bool, optional):
      Print the timestamp preceeded by a label string (default is True)

  Examples:
    >>> printProcessTime( 'od_process', True )
    Process: 'od_process'
    Started: Mon 20 Apr 2020, 17:23:32

    >>> printProcessTime( 'od_process', False )
    Process: 'od_process'
    Finished: Mon 20 Apr 2020, 17:23:37

    >>> printProcessTime( 'od_process', False, withprocline=False )
    Finished: Mon 20 Apr 2020, 17:23:37

  """
  if withprocline:
    procstr = 'Process: \'' + procnm + '\''
    print_fn( procstr )
  if isstart:
    retstr = 'Started:'
  else:
    retstr = 'Finished:'
  print_fn( retstr, sTimeUnitString() )

def kill( proc ):
  """Terminate a process

  Parameters:
    * proc (psutil.Popen): Object view of the process

  Returns:
    * Set and return subprocess.Popen.returncode

  """

  with proc.oneshot():
    proc.resume()
    proc.terminate()
    proc.wait(3)

def startAndWait( cmd ):
  """Run a command with subprocess

  Parameters:
    * cmd (list): command to be executed with its arguments.

  Returns:
    * str:
      stdout from the executed and retrieved subprocess.CompletedProcess

  Notes:
    * The command is executed by the run function of the subprocess module.
    * stdout is captured with a PIPE from the child process.
    * stderr is captured and forwarded to odpy.syslog_logger.

  """

  try:
    completedproc = subprocess.run( cmd, check=True, stdout=subprocess.PIPE, \
                                    stderr=get_std_stream() ).stdout
  except subprocess.CalledProcessError as err:
    std_msg( 'Failed: ', err )
    raise
  return completedproc

def log_subprocess_output(pipe):
    for line in iter(pipe.readline, b''):
        log_msg(line.decode('utf-8').strip('\n'))

def startDetached( cmd ):
  """Run a command in the background with psutil

  Parameters:
    * cmd (list): command to be executed with its arguments.

  Returns:
    psutil.Popen: Object view of the process

  Notes:
    * The command is executed by the Popen function of the psutil module.
    * Since the command is run as a daemon, it does not block the execution of the script.
    * stdout is captured and forwarded to odpy.proclog_logger.
    * stderr is captured and forwarded to odpy.proclog_logger.

  """

  try:
    runningproc = None
    if isWin():
      runningproc = psutil.Popen( cmd, stdout=subprocess.PIPE, \
                                       stderr=subprocess.STDOUT )
    else:
      runningproc = psutil.Popen( cmd, start_new_session=True, \
                                       stdout=subprocess.PIPE, \
                                       stderr=subprocess.STDOUT )
    t = threading.Thread(target=log_subprocess_output, args=(runningproc.stdout,))
    t.start()
  except subprocess.CalledProcessError as err:
    std_msg( 'Failed: ', err )
    raise
  return runningproc

