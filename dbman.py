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
  ret = json.loads( retstr )
  if ret['Status'] != 'OK':
    log_msg( ret['Status'] )
    raise FileNotFoundError
  return ret

def getByName( dblist, retname, keystr ):
  curentryidx = dblist['Names'].index( retname )
  return dblist[keystr][curentryidx]

def getDBKeyForName( dblist, retname ):
  return getByName( dblist, retname, 'IDs' )

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

def getFileLocation( dbkey, args=None ):
  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--json' )
  cmd.append( '--info' )
  cmd.append( dbkey )
  return retFileLoc( execCommand(cmd) )

def getNewEntryFileName( objnm, dirid, trgrp, trl, ext, ftype=None, args=None ):
  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--create' )
  cmd.append( objnm )
  cmd.append( dirid )
  cmd.append( trgrp )
  cmd.append( trl )
  cmd.append( ext )
  if lrntype != None:
      cmd.append( ftype )
  cmd.append( '--json' )
  return retFileLoc( execCommand(cmd) )

