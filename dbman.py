import sys
import json
from odpy.common import *
from subprocess import check_output,CalledProcessError

dbmanexe = 'od_DBMan'

def runDBCommand( cmd, stderrstrm ):
  ret = None
  try:
    ret = check_output( cmd, stderr=stderrstrm )
  except CalledProcessError as e:
    log_msg( 'Failed: ', e )
    raise FileNotFoundError
  return ret

def getDBList(args,translnm):
  cmd = getODCommand(args,dbmanexe)
  cmd.append( '--json' )
  cmd.append( '--list' )
  cmd.append( translnm )
  ret = runDBCommand( cmd, args['logfile'] )
  db = json.loads( ret.decode("utf-8") )
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
  fileloc = json.loads( bstdout.decode("utf-8") )
  if fileloc['Status'] != 'OK':
    log_msg( fileloc['Status'] )
    raise FileNotFoundError
  return fileloc['data']['File_name']

def getFileLocation( args, dbentry ):
  cmd = getODCommand(args,dbmanexe)
  cmd.append( '--json' )
  cmd.append( '--info' )
  cmd.append( dbentry['ID'] )
  return retFileLoc( runDBCommand(cmd,args['logfile']) )

def getNewEntryFileName( args, objnm, dirid, trgrp, trl, ext ):
  cmd = getODCommand(args,dbmanexe)
  cmd.append( '--create' )
  cmd.append( objnm )
  cmd.append( dirid )
  cmd.append( trgrp )
  cmd.append( trl )
  cmd.append( ext )
  cmd.append( '--json' )
  return retFileLoc( runDBCommand(cmd,args['logfile']) )

