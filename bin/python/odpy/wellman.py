import numpy as np
import json

import odpy.common as odcommon
import odpy.dbman as oddbman
from odpy.oscommand import getODCommand, execCommand

wellmanexe = 'od_WellMan'
wlldbdirid = '100050'
wlltrlgrp = 'Well'
dgbtrl = 'dGB'

dblist = None

def getNames( reload=False, args=None ):
  if args == None:
    args = odcommon.getODArgs()
  return getWellDBList(reload,args)['Names']

def getInfo( wllnm, reload=False, args=None ):
  ret = oddbman.getInfoByName( wllnm, wlltrlgrp,exenm=oddbman.dbmanexe, args=args )
  dbkey = getDBKey( wllnm, reload=reload, args=args )
  if dbkey != None:
      cmd = getODCommand(wellmanexe,args)
      cmd.append( '--info' )
      cmd.append( dbkey )
      try:
        wllinfo = oddbman.getDBDict( cmd )
        if isinstance(wllinfo,dict):
          for keynm in wllinfo:
            ret.update( {keynm: wllinfo[keynm]} )
      except:
        pass
  return ret

def getName( dbkey, reload=False, args=None ):
  cmd = getODCommand(wellmanexe,args)
  cmd.append( '--info' )
  cmd.append( dbkey )
  ret = oddbman.getDBDict( cmd )
  return ret['Name']

def getLogNames( wllnm, reload=False, args=None ):
  dbkey = getDBKey( wllnm, reload=reload, args=args )
  cmd = getODCommand(wellmanexe,args)
  cmd.append( '--list-logs' )
  cmd.append( dbkey )
  ret = oddbman.getDBDict( cmd )
  return ret['Names']

def getLog( wllnm, lognm, reload=False, args=None ):
  """Get a well log from the OpendTect database
    Read a single log from the OpendTect database, with
    any depth resampling or unit conversion.
    
    Args:
      wllnm (string): Well database name
      lognm (string): Log name as reported by getLogNames(wllnm)
      reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
      args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.
          
    Returns:
      tuple: Two arrays with depths (MD) and log values

    
  """
  dbkey = getDBKey( wllnm, reload=reload, args=args )
  cmd = getODCommand(wellmanexe,args)
  cmd.append( '--read-log' )
  cmd.append( dbkey )
  cmd.append( lognm )
  ret = oddbman.getDBDict( cmd )
  return (ret['MDs'], ret['Values'])

def getDBKey( wllnm, reload=False, args=None ):
  global dblist
  dblist = getWellDBList(reload,args)
  return oddbman.getDBKeyForName( dblist, wllnm )  

def getWellDBList( reload, args=None ):
  global dblist
  if dblist != None and not reload:
    return dblist

  if args == None:
    args = odcommon.getODArgs()
  dblist = oddbman.getDBList(wlltrlgrp,exenm=wellmanexe,args=args)
  return dblist

def getMarkers( wllnm, reload=False, args=None ):
  dbkey = getDBKey( wllnm, reload=reload, args=args )
  cmd = getODCommand(wellmanexe,args)
  cmd.append( '--list-markers' )
  cmd.append( dbkey )
  ret = oddbman.getDBDict( cmd )
  return (ret['Names'], ret['MDs'], ret['Color'])

def getTrack( wllnm, reload=False, args=None ):
  dbkey = getDBKey( wllnm, reload=reload, args=args )
  cmd = getODCommand(wellmanexe, args)
  cmd.append( '--read-track' )
  cmd.append( dbkey )
  ret = oddbman.getDBDict( cmd )
  return (ret['MDs'], ret['TVDs'], ret['X-Coords'], ret['Y-Coords'])

