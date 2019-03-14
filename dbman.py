import sys
import json
import platform
from odpy.common import *
from subprocess import check_output,CalledProcessError

dbmanexe = 'od_DBMan'

def runDBCommand( cmd, args=None ):
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

def getDBList(translnm,args=None):
  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--json' )
  cmd.append( '--list' )
  cmd.append( translnm )
  ret = runDBCommand( cmd, args )
  retstr = ret.decode('utf-8')
  if platform.system() == 'Windows':
    retstr = retstr.translate(str.maketrans({"\\": r"\\"}))
  db = json.loads( retstr )
  if db['Status'] != 'OK':
    log_msg( db['Status'] )
    raise FileNotFoundError
  return db['data']

def getByName( dblist, retname ):
  for entry in dblist:
    if entry['Name'] == retname:
      return entry
  return None

def retFileLoc( bstdout ):
  if bstdout == None:
    return None
  retstr = bstdout.decode('utf-8')
  if platform.system() == 'Windows':
      retstr = retstr.translate(str.maketrans({"\\": r"\\"}))
  fileloc = json.loads( retstr )
  if fileloc['Status'] != 'OK':
    log_msg( fileloc['Status'] )
    raise FileNotFoundError
  return fileloc['data']['File_name']

def getFileLocation( dbentry, args=None ):
  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--json' )
  cmd.append( '--info' )
  cmd.append( dbentry['ID'] )
  return retFileLoc( runDBCommand(cmd,args) )

def getNewEntryFileName( objnm, dirid, trgrp, trl, ext, args=None ):
  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--create' )
  cmd.append( objnm )
  cmd.append( dirid )
  cmd.append( trgrp )
  cmd.append( trl )
  cmd.append( ext )
  cmd.append( '--json' )
  return retFileLoc( runDBCommand(cmd,args) )

