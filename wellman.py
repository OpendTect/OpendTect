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
  dbkey = getDBKey( wllnm, reload=reload, args=args )
  return oddbman.getInfoByName( dbkey, wlltrlgrp,exenm=wellmanexe, args=args )

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

