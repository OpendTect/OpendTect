import json

from odpy.common import isWin, log_msg
from odpy.oscommand import getODCommand, execCommand

dbmanexe = 'od_DBMan'

def getDBList(translnm,alltrlsgrps=False,exenm=dbmanexe,args=None):
  cmd = getODCommand(exenm,args)
  cmd.append( '--json' )
  if alltrlsgrps:
    cmd.append( '--all' )
  cmd.append( '--list' )
  cmd.append( translnm )
  return getDBDict( cmd )

def getInfoFromDBListByNameOrKey(nm_or_key,dblist):
  for i in range(len(dblist['Names'])):
    dbobjnm = dblist['Names'][i]
    objid = dblist['IDs'][i]
    objfmt = dblist['Formats'][i]
    if dbobjnm != nm_or_key and objid != nm_or_key:
      continue
    if 'TranslatorGroups' in dblist:
      objtrlgrp = dblist['TranslatorGroups'][i]
    if 'Types' in dblist:
      objtyp = dblist['Types'][i]
    ret = {
      'ID': objid,
      'Name': dbobjnm,
      'Format': objfmt,
    }
    if len(objtrlgrp) > 0:
      ret.update({'TranslatorGroup': objtrlgrp})
    if len(objtyp) > 0:
      ret.update({'Type': objtyp})
    return ret

def getInfoByName(objnm,translnm,exenm=dbmanexe,args=None ):
  cmd = getODCommand(exenm,args)
  cmd.append( '--json' )
  cmd.append( '--exists' )
  cmd.append( objnm )
  cmd.append( '--trl-grp' )
  cmd.append( translnm )
  ret = getDBDict( cmd )
  if not 'ID' in ret:
    return None
  return ret

def getInfoByKey(objkey,exenm=dbmanexe,args=None ):
  cmd = getODCommand(exenm,args)
  cmd.append( '--json' )
  cmd.append( '--info' )
  cmd.append( objkey )
  return getDBDict( cmd )

def getDBDict( cmd ):
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
  if ftype != None:
      cmd.append( ftype )
  cmd.append( '--json' )
  return retFileLoc( execCommand(cmd) )

