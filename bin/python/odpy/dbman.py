import json

from odpy.common import isWin, log_msg
from odpy.oscommand import getODCommand, execCommand

dbmanexe = 'od_DBMan'

def getDBList(translnm,args=None):
  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--json' )
  cmd.append( '--list' )
  cmd.append( translnm )
  ret = execCommand( cmd )
  retstr = ret.decode('utf-8')
  if isWin():
    retstr = retstr.translate(str.maketrans({"\\": r"\\"}))
  db = json.loads( retstr )
  if db['Status'] != 'OK':
    log_msg( db['Status'] )
    raise FileNotFoundError
  return db['Data']

def getByName( dblist, retname ):
  for entry in dblist:
    if entry['Name'] == retname:
      return entry
  return None

def retFileLoc( bstdout ):
  if bstdout == None:
    return None
  retstr = bstdout.decode('utf-8')
  if isWin():
    retstr = retstr.translate(str.maketrans({"\\": r"\\"}))
  fileloc = json.loads( retstr )
  if fileloc['Status'] != 'OK':
    log_msg( fileloc['Status'] )
    raise FileNotFoundError
  return fileloc['File_name']

def getFileLocation( dbentry, args=None ):
  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--json' )
  cmd.append( '--info' )
  cmd.append( dbentry['ID'] )
  return retFileLoc( execCommand(cmd) )

def getNewEntryFileName( objnm, dirid, trgrp, trl, ext, args=None ):
  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--create' )
  cmd.append( objnm )
  cmd.append( dirid )
  cmd.append( trgrp )
  cmd.append( trl )
  cmd.append( ext )
  cmd.append( '--json' )
  return retFileLoc( execCommand(cmd) )

