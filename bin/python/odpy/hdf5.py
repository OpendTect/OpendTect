"""
Copyright (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  * AUTHOR : A. Huck
  * DATE   : Nov 2018

Module Summary
###############

Tools for reading hdf5 files created by OpendTect. Ensures standard access to hdf5
for  independent workflow

KEY methods
-------------

* openFile()

  * opens any h5 or hdf5 files, ensures and optimize access to the file

Example
--------
>>> import odpy.hdf5 as odhdf
>>> file =odhdf.openFile(filenm='DTECT_DATA/F3_Demo_2020/NLAs/3D_seismic_energy.h5', mode='r')
>>> file
    <HDF5 file "3D_seismic_energy.h5" (mode r)>

>>> info = odhdf.getInfoDataSet(h5file_or_grp=file)
>>> info
    <HDF5 dataset "++info++": shape (1,), type "|S1">

"""

import collections
import numpy as np
import h5py

dGBInfoDSName = '++info++'

def openFile( filenm, mode ):
  """ Opens and ensures and optimize access to the hdf5 file

  Parameters:
    * filenm (str): HDF5 file name
    * mode (str): options to read, write or edit to the file (r, r+, w, w-, x, a)

  Returns:
    * obj: A HDF5 file object (in mode selected)
  """
  
  return h5py.File( filenm, mode, libver=('v110', 'latest') )

def getInfoDataSet( h5file_or_grp ):
  """ Gets info on HDF5 object

  Parameters:
    * h5file_or_grp (object): hdf5 file object

  Returns:
    * obj: A h5 dataset object created by OpendTect,
           Input error is generated if HDF5 file is not created by OpendTect

  Notes:
    * KeyError (HDF5 key not found) is returned if dataset has no attribute passed           
  """

  try:
    return h5file_or_grp[dGBInfoDSName]
  except KeyError:
    print( "Input Error: HDF5 file probably not created by OpendTect (no ++info++ attribute)" )
    raise

def ensureHasDataset( h5file_or_grp ):
  """ Ensures HDF5 object has info file (++info++) created by OpendTect

  Parameters:
    * h5file_or_grp (obj): HDF5 file object

  Returns:
    * obj: A h5 dataset object created by OpendTect
  """

  if dGBInfoDSName in h5file_or_grp:
    return getInfoDataSet( h5file_or_grp )
  return h5file_or_grp.create_dataset(dGBInfoDSName,dtype='S1')

def getAttr( dataset, ky ):
  """ Gets attribute value from HDF5 Dataset 

  Parameters:
    * dataset (obj): HDF5 file object
    * ky (str): HDF5 attribute name

  Returns:
    bytes: Attribute value

  Notes:
    * KeyError (HDF5 key not found) is returned if dataset has no attribute passed
  """

  try:
    attrib = dataset.attrs[ky]
    return np.bytes_(dataset.attrs[ky]).decode().rstrip().split("\x00")[0]
  except KeyError:
    print( "HDF5 key not found: '"+ky+"'. Available keys:\n")
    print( list(dataset.attrs.keys()) )
    raise

def setAttr( dataset, ky, str ):
  """ Sets HDF5 dataset attribute

  Parameters:
    * dataset (obj): HDF5 file object
    * ky (str): attribute name
    * str (str): attribute value

  Returns:
    * Nothing (sets dataset attribute)
  """

  dataset.attrs[ky] = np.chararray.encode( str )

def hasAttr( dataset, ky ):
  """ Checks if HDF5 object has attribute

  Parameters:
    * dataset (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * bool: True if attribute is present, False if otherwise
  """

  return ky in dataset.attrs

def getAttrs( dataset, ky ):
  """ Gets attribute value(s) from HDF5 dataset

  Parameters:
    * dataset (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * list: of atrribute value(s)

  Notes:
    * KeyError (HDF5 key not found) is returned if dataset has no attribute passed
      List of available attributes is displayed
  """

  return getAttr( dataset, ky ).split( "`" )

def getText( infos, ky ): 
  """ Gets attribute text value from HDF5 dataset 

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * str: attribute value
  """

  ret = getAttr( infos, ky )
  if "`" in ret:
    return ret.split( "`" )
  return ret

def setArray( infos, ky, arr ):
  """ Sets array values for dataset attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name
    * arr (list, tup, int, float, str): attribute value(s)

  Returns:
    * Nothing (sets dataset attribute value(s))
  """

  try:
    arrstr = '`'.join( map(str,arr) )
  except TypeError:
    arrstr = str(arr)
  setAttr( infos, ky, arrstr )

def getBoolValue( infos, ky ):
  """ Gets boolean value

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    bool: True if attribute key evaluates to 'Yes', False if otherwise
  """

  return getAttr( infos, ky ) == "Yes"

def getIArray_( valsstr ):
  """ Provides integer equivalence of input. Converts input or element of input into integers

  Parameters:
    * valsstr (int, float, list of int/float)

  Returns:
    * int, list of int
  """

  if isinstance(valsstr,list) and len(valsstr) > 1:
    ret = list()
    for valstr in valsstr:
      ret.append( int(valstr) )
    return ret
  try:
    return int(valsstr)
  except ValueError:
    return int(float(valsstr))

def getIntValue( infos, ky ):
  """ Gets integer value of attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * int: attribute value

  Notes:
    * Returns ValueError if attribute value is text
  """

  return getIArray_( getText(infos,ky) )

def getIInterval( infos, ky ):
  """ Gets interval int values of attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * list: interval int values

  Notes:
    * Returns ValueError if attribute value is text
  """

  return getIArray_( getText(infos,ky) )

def getIStepInterval( infos, ky ):
  """ Gets interval step int values of attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * tuple: interval step int values

  Notes:
    * Returns ValueError if attribute value is text
  """

  return getIArray_( getText(infos,ky) )

def getIArray( infos, ky ):
  """ Gets array int values of attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * list: attribute int values

  Notes:
    * Returns ValueError if attribute value is text
  """

  return getIArray_( getText(infos,ky) )

def getDArray_( valsstr ):
  """ Provides float equivalence of input. Converts input or element of input into floats

  Parameters:
    * valsstr (int, float, list of int/float)

  Returns:
    * float, list of float
  """

  if isinstance(valsstr,list) and len(valsstr) > 1:
    ret = list()
    for valstr in valsstr:
      ret.append( float(valstr) )
    return ret
  return float(valsstr)
  
def getDValue( infos, ky ):
  """ Gets float value of attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * float: attribute value

  Notes:
    * Returns ValueError if attribute value is text
  """
  return getDArray_( getText(infos,ky) )

def getDInterval( infos, ky ):
  """ Gets interval float values of attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * list: interval float values

  Notes:
    * Returns ValueError if attribute value is text
  """

  return getDArray_( getText(infos,ky) )

def getDStepInterval( infos, ky ):
  """ Gets float interval step values of attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * tuple: interval step float values

  Notes:
    * Returns ValueError if attribute value is text
  """

  return getDArray_( getText(infos,ky) )

def getDArray( infos, ky ):
  """ Gets array float values of attribute

  Parameters:
    * infos (obj): HDF5 file object
    * ky (str): attribute name

  Returns:
    * list: attribute float values

  Notes:
    * Returns ValueError if attribute value is text
  """

  return getDArray_( getText(infos,ky) )

def getAttribInfo( filenm ):
  h5file = openFile( filenm, 'r' )
  infods = getInfoDataSet( h5file )

  blocksversion = getIntValue(infods,"Blocks.Version")
  zdomain  = getText(infods,"ZDomain")
  datasetnm = getText(infods,"Name.Cube")
  compnms  = getText(infods,"Components")
  datachar = getText(infods,"Data storage")
  inlinerg    = getIInterval(infods,"In-line range")
  crosslinerg = getIInterval(infods,"Cross-line range")
  zrg      = getDStepInterval(infods,"Z range")
  inl_step = getIntValue(infods,"Step In-line")
  crl_step = getIntValue(infods,"Step Cross-line")
  blocksdim = getIInterval(infods,"Blocks.Max Dimensions")
  blocksinlrg = getIInterval(infods,"Blocks.Inl ID Range")
  blockscrlrg = getIInterval(infods,"Blocks.Crl ID Range")
  h5file.close()
  if zdomain == "TWT":
    zrg = np.multiply(zrg,1000).astype("int32")
    zrg = [zrg[0],zrg[1],zrg[2]]
  return collections.OrderedDict({
    'name': datasetnm,
    'attributes': compnms,
    'version': blocksversion,
    'zdomain': zdomain,
    'storage': datachar,
    'range': collections.OrderedDict({
      'Inline': [inlinerg[0],inlinerg[1],inl_step],
      'Crossline': [crosslinerg[0],crosslinerg[1],crl_step],
      'Z': zrg
    }),
    'block': {
      'size': blocksdim,
      'range': collections.OrderedDict({
        'Inline': blocksinlrg,
        'Crossline': blockscrlrg
      })
    }
  })

# Survey transformation (X-Y) to (Inl-Crl)
# Given an Inl-Crl pair: ic_pos = [inl,crl]
# the following returns the corresponding X,Y location:
# xy_pos = transform['origin'] + np.dot( transform['rotation'], ic_pos )
def getTransform( info ):
  coordsxbid = getDStepInterval(info,"Coord-X-BinID")
  coordsybid = getDStepInterval(info,"Coord-Y-BinID")
  return collections.OrderedDict({
    'origin':   [coordsxbid[0],coordsybid[0]],
    'rotation': [[coordsxbid[1],coordsxbid[2]],[coordsybid[1],coordsybid[2]]]
  })

def getSurveyInfo( filenm ):
  """ Gets survey information

  Parameters:
    * filenm (str): file name

  Returns:
    * orderedDict: containing information on survey name, survey ranges (inline, crossline, x, y), transform
  """

  h5file = openFile( filenm, 'r' )
  infods = getInfoDataSet( h5file )
  surveyname = getText(infods,"Name.Survey")
  inlstart = getIntValue(infods,"First In-line")
  inlstop  = getIntValue(infods,"Last In-line")
  crlstart = getIntValue(infods,"First Cross-line")
  crlstop  = getIntValue(infods,"Last Cross-line")
  xrg = getDInterval(infods,"X range")
  yrg = getDInterval(infods,"Y range")
  transform = getTransform( infods )
  h5file.close()
  return collections.OrderedDict({
    'name': surveyname,
    'range': collections.OrderedDict({
      'Inline': [inlstart,inlstop],
      'Crossline': [crlstart,crlstop],
      'X': xrg,
      'Y': yrg
    }),
    'transform': transform
  })
