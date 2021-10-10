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
  """ Gets survey well names from database

  Parameters:
    * reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
    * args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

  Returns:
    * list: list of available survey wells from database


  """

  if args == None:
    args = odcommon.getODArgs()
  return getWellDBList(reload,args)['Names']

def getInfo( wllnm, reload=False, args=None ):
  """ Gets information for a well

  Parameters:
    * wllnm (str): well name from survey
    * reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
    * args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

  Returns:
    * dict: information on well ID, name, x y cordinates, status etc

  """

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
  """ Gets well name

  Parameters:
    * dbkey (str): well database key
    * reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
    * args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

  Returns:
    * str: Well name


  """

  cmd = getODCommand(wellmanexe,args)
  cmd.append( '--info' )
  cmd.append( dbkey )
  ret = oddbman.getDBDict( cmd )
  return ret['Name']

def getLogNames( wllnm, reload=False, args=None ):
  """ Gets logs available for a well

  Paramters:
    * wllnm (str): name of well
    * reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
    * args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

  Returns:
    * list: list containing log names

  """

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
    
    Parameters:
      * wllnm (string): Well database name
      * lognm (string): Log name as reported by getLogNames(wllnm)
      * reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
      * args (dictionary, optional): Dictionary of optional parameters (see common).
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

def getLogs( wllnm, logidxlst, zstep=0.5, reload=False, args=None ):
  """Get re-sampled logs from OpendTect

  Parameters:
      wllnm (string): Well database name
      logidxlst (string): List of log indices to be resampled
      zstep (double, optional): Resampling step in meters. Default to 0.5
      reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
      args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

  Returns:
    dict: Dictionary with log names as keys, logs as numpy arrays.
        A depth array is also always output.

  Example:
     Get the logs from the well F03-4 with database indices 0, 1, 4, 6:
     >>> logs = odpy.wellman.getLogs( 'F03-4', '0`1`4`6' )
     
     Undefined log samples will get the value 1e30 (not NaN)


  """
  dbkey = getDBKey( wllnm, reload=reload, args=args )
  cmd = getODCommand(wellmanexe,args)
  cmd.append( '--read-logs' )
  cmd.append( dbkey )
  cmd.append( logidxlst )
  cmd.append( '--zstep' )
  cmd.append( str(zstep) )
  ret = oddbman.getDBDict( cmd )
  nlogs = len(ret['Names'])
  result = { 'depth': ret['MDs'] }
  for n in range(nlogs):
    result[ret['Names'][n]] = ret[f'Log_{n}']
  return result

def getDBKey( wllnm, reload=False, args=None ):
  """ Gets well database key

  Parameters:
    * wllnm (str): well name
    * reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
    * args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

  Returns:
    str: Database key for well name provided
  """

  global dblist
  dblist = getWellDBList(reload,args)
  return oddbman.getDBKeyForName( dblist, wllnm )  

def getWellDBList( reload, args=None ):
  """ Gets information on wells from database for a survey

  Parameters:
    * reload (boolean, optional): Force re-reading of the database files (no caching allowed)
    * args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

  Returns:
    dict: containing information on survey database wells (size, IDs, Names, Status, etc)
  """

  global dblist
  if dblist != None and not reload:
    return dblist

  if args == None:
    args = odcommon.getODArgs()
  dblist = oddbman.getDBList(wlltrlgrp,exenm=wellmanexe,args=args)
  return dblist

def getMarkers( wllnm, reload=False, args=None ):
  """ Gets information on available markers for a well

  Parameters:
    * wllnm (str): well name
    * reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
    * args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

  Returns:
    tuple: contains lists of available markers (names, MDs, colors)
  """

  dbkey = getDBKey( wllnm, reload=reload, args=args )
  cmd = getODCommand(wellmanexe,args)
  cmd.append( '--list-markers' )
  cmd.append( dbkey )
  ret = oddbman.getDBDict( cmd )
  return (ret['Names'], ret['MDs'], ret['Color'])

def getTrack( wllnm, reload=False, args=None ):
  """ Gets well (track) depth information

  Parameters:
    * wllnm (str): well name
    * reload (boolean, optional): Force re-reading of the database files
          (no caching allowed). Default to False
    * args (dictionary, optional): Dictionary of optional parameters (see common).
          Default to None.

    Returns:
      tuple: contains lists of track depths (MDs, TVDs, x cord., y cord.)

  """

  dbkey = getDBKey( wllnm, reload=reload, args=args )
  cmd = getODCommand(wellmanexe, args)
  cmd.append( '--read-track' )
  cmd.append( dbkey )
  ret = oddbman.getDBDict( cmd )
  return (ret['MDs'], ret['TVDs'], ret['X-Coords'], ret['Y-Coords'])

