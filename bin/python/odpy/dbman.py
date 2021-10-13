import json

from odpy.common import isWin, log_msg
from odpy.oscommand import getODCommand, execCommand

dbmanexe = 'od_DBMan'

def getDBList(translnm,alltrlsgrps=False,exenm=dbmanexe,args=None):
  """ Gets information on survey database wells

  Parameters:
    * translnm (string): default value='Well'
    * alltrlsgrps (bool): if True, returns information on TranslatorGroups for available wells
    * exenm (string): database executable file
    * args (dict, optional):
      Dictionary with the members 'dtectdata' and 'survey' as 
      single element lists, and/or 'dtectexec' (see odpy.common.getODSoftwareDir)

  Returns:
    * dict: Dictionary containing database survey well information (size, IDs, Names, Formats, Status)
  """

  cmd = getODCommand(exenm,args)
  cmd.append( '--json' )
  if alltrlsgrps:
    cmd.append( '--all' )
  cmd.append( '--list' )
  cmd.append( translnm )
  return getDBDict( cmd )

def getInfoFromDBListByNameOrKey(nm_or_key,dblist):
  """ Gets info from database list with obj key or name

  Parameters:
    * nm_or_key (str): object key or name
    * dblist (dict): survey database list, check odpy.getDBList for docs

  Returns:
    * dict: info on database object (Name,ID, Format, Type, TranslatorGroup iff available)
  """

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
  """ Gets object info by name

  Parameters:
    * objnm (str): database object to get info on
    * translnm (str):
    * exenm (str): executable file, defaults to dbmanexe=od_DBMan
    * args (dict, optional):
      Dictionary with the members 'dtectdata' and 'survey' as 
      single element lists, and/or 'dtectexec' (see odpy.common.getODSoftwareDir)

  Returns:
    * dict: information on object, dict keys include; ID, Name, file name, etc

  Example:
  >>> import odpy.dbman as dbman
  >>> dbman.getInfoByName(objnm='F02-1', translnm='Well')
      {'ID': '100050.2',
       'Name': 'F02-1',
       'Format': 'dGB',
       'TranslatorGroup': 'Well',
       'File_name': 'C:\\Users\\OLAWALE IBRAHIM\\DTECT_DATA\\F3_Demo_2020\\WellInfo\\F02-1.well',
       'Status': 'OK'}

  """

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
  """ Gets datbase info on well

  Parameters:
    * objkey (str): well ID key
    * exenm (str): executable file name
    * args (dict, optional):
      Dictionary with the members 'dtectdata' and 'survey' as 
      single element lists, and/or 'dtectexec' (see odpy.common.getODSoftwareDir)

  Returns:
    dict: file info (ID, Name, Format, File name, etc)
  """

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
  """ Gets value of specified database list  key

  Parameters:
    * dblist (dict): survey database list, check odpy.getDBList for docs
    * retname (str): key to return from dblist
    * keystr (str): value to return from retname

  Returns:
    * str: database object value

  Example:
  
  >>> import odpy.dbman as dbman
  >>> dbman.getByName(dblist, 'F03-4', 'IDs')
      '100050.4'

  """

  curentryidx = dblist['Names'].index( retname )
  return dblist[keystr][curentryidx]

def getDBKeyForName( dblist, retname ):
  """ Gets object ID key from database info

  Parameters:
    * dblist (dict): survey database list, check odpy.getDBList for docs
    * retname (str): key to return from dblist

  Returns:
    * str: ID of database object (well)

  """

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
  """  Gets full file path

  Parameters:
    * dbkey (str): object database key
    * args (dict, optional):
      Dictionary with the members 'dtectdata' and 'survey' as 
      single element lists, and/or 'dtectexec' (see odpy.common.getODSoftwareDir)

  Returns:
    str: full path to file

  """

  cmd = getODCommand(dbmanexe,args)
  cmd.append( '--json' )
  cmd.append( '--info' )
  cmd.append( dbkey )
  return retFileLoc( execCommand(cmd) )

def getNewEntryFileName( objnm, dirid, trgrp, trl, ext, ftype=None, args=None ):
  """ Registers a new OpendTect dataset to database

  Parameters:
    * objnm (str): file name
    * dirid  (internal: int)
    * trgrp (str): TranslatorGroup e.g. Well, Seismic, etc
    * ext (str): file extension

  Returns:
    * file path to the object created with write permission

  """

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

