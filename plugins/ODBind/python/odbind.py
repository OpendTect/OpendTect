"""Python CTypes bindings to OpendTect

Copyright (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt

Module Summary
###############

The OpendTect C++ ODBind plugin provides an FFI (foreign function interface) compatible interface to data within 
an OpendTect survey/project. The "odbind" Python module is a Python ctypes binding and object oriented wrapper 
to the ODBind plugin.


Python Classes
-------------
Survey
    Encapsulates an OpendTect survey/project, providing information about the survey. Survey objects are used as context
    to construct the various data objects. The Survey class also includes a number of static class methods that provide
    information about all or a user speciifed subset of the surveys within a specific OpendTect data folder.

Horizon2D
    Encapsulates an OpendTect 2D seismic horizon.

Horizon3D
    Encapsulates an OpendTect 3D seismic horizon.

Seismic2D
    Encapsulates an OpendTect 2D seismic dataset.

Seismic3D
    Encapsulates an OpendTect 3D seismic dataset.

Well
    Encapsulates an OpendTect well

Examples
--------

"""
import sys, os
import ctypes as ct
import json
import odpy.common as odc

def get_lib_name(modnm: str) -> str:
    ''' Get platform dependent shared library name for given module '''
    libnm = str()
    if sys.platform == 'win32':
        libnm = modnm + '.dll'
    else:
        libnm = 'lib' + modnm
        if sys.platform == 'darwin':
            libnm = libnm + '.dylib'
        else:
            libnm = libnm + '.so'
    
    return libnm

def wrap_function(lib, funcname, restype, argtypes):
    ''' Simplify wrapping ctypes functions '''
    func = lib.__getattr__(funcname)
    func.restype = restype
    func.argtypes = argtypes
    return func

libodbind = os.path.join(odc.getExecPlfDir(), get_lib_name('ODBind'))
LIBODB = ct.CDLL(libodbind)
init_module = wrap_function(LIBODB, 'initModule', None, [ct.c_char_p])
init_module(libodbind.encode())

cstring_del = wrap_function(LIBODB, 'cstring_del', None, [ct.POINTER(ct.c_char_p)])
def pystr(cstringptr: ct.POINTER(ct.c_char_p), autodel: bool=True) ->str:
    pystring = ct.cast(cstringptr, ct.c_char_p).value.decode()
    if autodel:
        cstring_del(cstringptr)
    return pystring

stringset_new = wrap_function(LIBODB, 'stringset_new', ct.c_void_p, [])
stringset_copy = wrap_function(LIBODB, 'stringset_copy', ct.c_void_p, [ct.c_void_p])
stringset_del = wrap_function(LIBODB, 'stringset_del', None, [ct.c_void_p])
stringset_size = wrap_function(LIBODB, 'stringset_size', ct.c_int, [ct.c_void_p])
stringset_add = wrap_function(LIBODB, 'stringset_add', ct.c_void_p, [ct.c_void_p, ct.c_char_p])
stringset_get = wrap_function(LIBODB, 'stringset_get', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_int])
def makestrlist(inlist: list) ->ct.c_void_p:
    res = stringset_new()
    for val in inlist:
        stringset_add(res, val)
    return res

def pystrlist(stringsetptr: ct.c_void_p, autodel: bool=True) ->list[str]:
    res = []
    for idx in range(stringset_size(stringsetptr)):
        res.append(pystr(stringset_get(stringsetptr, idx), False))
    if autodel:
        stringset_del(stringsetptr)
    return res

def pyjsonstr(jsonstrptr: ct.POINTER(ct.c_char_p), autodel: bool=True):
    res = json.loads(ct.cast(jsonstrptr, ct.c_char_p).value.decode())
    if autodel:
        cstring_del(jsonstrptr)
    return res


class Survey(object):
    """A class for an OpendTect survey/project.

    """
    _new = wrap_function(LIBODB, 'survey_new', ct.c_void_p, [ct.c_char_p, ct.c_char_p])
    _del = wrap_function(LIBODB, 'survey_del', None, [ct.c_void_p])
    _bin = wrap_function(LIBODB, 'survey_bin', None, [ct.c_void_p, ct.c_double, ct.c_double, ct.POINTER(ct.c_int), ct.POINTER(ct.c_int)])
    _bincoords = wrap_function(LIBODB, 'survey_bincoords', None, [ct.c_void_p, ct.c_double, ct.c_double, ct.POINTER(ct.c_double), ct.POINTER(ct.c_double)])
    _coords = wrap_function(LIBODB, 'survey_coords', None, [ct.c_void_p, ct.c_int, ct.c_int, ct.POINTER(ct.c_double), ct.POINTER(ct.c_double)])
    _crs = wrap_function(LIBODB, 'survey_crs', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _feature = wrap_function(LIBODB, 'survey_feature', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _features = wrap_function(LIBODB, 'survey_features', ct.POINTER(ct.c_char_p), [ct.c_char_p, ct.c_void_p])
    _has2d = wrap_function(LIBODB, 'survey_has2d', ct.c_bool, [ct.c_void_p])
    _has3d = wrap_function(LIBODB, 'survey_has3d', ct.c_bool, [ct.c_void_p])
    _hasobject = wrap_function(LIBODB, 'survey_hasobject', ct.c_bool, [ct.c_void_p, ct.c_char_p, ct.c_char_p])
    _info = wrap_function(LIBODB, 'survey_info', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _infos = wrap_function(LIBODB, 'survey_infos', ct.POINTER(ct.c_char_p), [ct.c_char_p, ct.c_void_p])
    _name = wrap_function(LIBODB, 'survey_name', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _names = wrap_function(LIBODB, 'survey_names', ct.c_void_p, [ct.c_char_p])
    _path = wrap_function(LIBODB, 'survey_path', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _type = wrap_function(LIBODB, 'survey_survtype', ct.POINTER(ct.c_char_p), [ct.c_void_p])

    def __init__(self, basedir, survey_name):
        """Initialise an OpendTect survey object

        Parameters
        ----------
        basedir : str
            OpendTect root data folder
        survey_name : str
            OpendTect survey name

        """
        self._survey = Survey._new(basedir.encode(), survey_name.encode())

    def __del__(self):
        Survey._del(self._survey)

    @property
    def crs(self) ->str:
        """str: Survey crs code (readonly)"""
        return pystr(Survey._crs(self._survey))

    @property
    def has2d(self) ->bool:
        """bool: True if the survey contains 2D data, False otherwise (readonly)"""
        return Survey._has2d(self._survey)

    @property
    def has3d(self) ->bool:
        """bool: True if the survey contains 3D data, False otherwise (readonly)"""
        return Survey._has3d(self._survey)

    @property
    def name(self) ->str:
        """str: OpendTect survey name (readonly)"""
        return pystr(Survey._name(self._survey))

    @property
    def survey_type(self) ->str:
        """str: Survey type: one of 2D, 3D or 2D3D (readonly)"""
        return pystr(Survey._type(self._survey))

    def bin(self, x: float, y: float ) ->tuple[int, int]:
        """Return the nearest inline and crossline location to the given X and Y coordinates.

        Parameters
        ----------
        x : float
            x or easting in the survey's coordinate system
        y : float
            y or northing  in the survey's coordinate system

        Returns
        -------
        tuple[int, int]
            The nearest inline and crossline location to the given X and Y coordinates

        """ 
        iline = ct.c_int()
        xline = ct.c_int()
        Survey._bin(self._survey, x, y, ct.byref(iline), ct.byref(xline))
        return (iline.value, xline.value)

    def bincoords(self, x: float, y: float ) ->tuple[float, float]:
        """Return the decimal inline and crossline location of the given X and Y coordinates.

        Parameters
        ----------
        x : float
            x or easting in the survey's coordinate system
        y : float
            y or northing  in the survey's coordinate system

        Returns
        -------
        tuple[float, float]
            The decimal inline and crossline location equivalent to the given X and Y coordinates

        """
        iline = ct.c_double()
        xline = ct.c_double()
        Survey._bincoords(self._survey, x, y, ct.byref(iline), ct.byref(xline))
        return (iline.value, xline.value)

    def coords(self, iline: int, xline: int ) ->tuple[float, float]:
        """Return the X and Y coordinates corresponding to the	given inline and crossline.

        Parameters
        ----------
        iline : int
            Inline number
        xline : intfloat
            Crossline number

        Returns
        -------
        tuple[float, float]
            The X and Y coordinates equivalent to the given inline and crossline

        """
        x = ct.c_double()
        y = ct.c_double()
        Survey._coords(self._survey, iline, xline, ct.byref(x), ct.byref(y))
        return (x.value, y.value)

    def feature(self) ->dict:
        """ Return a GeoJSON feature for the survey

        """
        return pyjsonstr(Survey._feature(self._survey))

    def info(self) ->dict:
        """Return basic information for the OpendTect survey.
        
        Returns
        -------
        dict

        """
        return pyjsonstr(Survey._info(self._survey))

    @staticmethod
    def names(basedir: str) ->list[str]:
        """ Return the names of all surveys in the given data root

        Parameters
        ----------
        basedir : str
            The OpendTect data root

        Returns
        -------
        list[str]

        """
        return pystrlist(Survey._names(basedir.encode()))

    @staticmethod
    def infos(basedir: str, fornms: list=[]) ->dict:
        """ Return basic information for all or a subset of surveys in the given data root.

        Parameters
        ----------
        basedir : str
            The OpendTect data root
        fornms : list[str]
            A list of survey names to use, an empty list will give information for all surveys.
            
        Returns
        -------
        dict

        """ 
        fornmsptr = stringset_new()
        for nm in fornms:
            stringset_add(fornmsptr, nm.encode())
        infolist = pyjsonstr(Survey._infos(basedir.encode(), fornmsptr ))
        stringset_del(fornmsptr)
        return {key: [i[key] for i in infolist] for key in infolist[0]}

    @staticmethod
    def features(basedir: str, fornms: list=[]) ->str:
        """ Return a GeoJSON Feature Collection with the outlines and basic information for all 
        or a subset of surveys in the given data root.

        Parameters
        ----------
        basedir : str
            The OpendTect data root
        fornms : list[str]
            A list of survey names to use, an empty list will give information for all surveys.
            
        Returns
        -------
        dict

        """ 
        fornmsptr = stringset_new()
        for nm in fornms:
            stringset_add(fornmsptr, nm.encode())
        res = pystr(Survey._features(basedir.encode(), fornmsptr ))
        stringset_del(fornmsptr)
        return res


class Horizon2D(object):
    """
    A class for an OpendTect 2D horizon
    """

    _new = wrap_function(LIBODB, 'horizon2d_new', ct.c_void_p, [ct.c_void_p, ct.c_char_p])
    _del = wrap_function(LIBODB, 'horizon2d_del', None, [ct.c_void_p])
    _attribcount = wrap_function(LIBODB, 'horizon2d_attribcount', ct.c_int, [ct.c_void_p])
    _attribnames = wrap_function(LIBODB, 'horizon2d_attribnames', ct.c_void_p, [ct.c_void_p])
    _info = wrap_function(LIBODB, 'horizon2d_info', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _infos = wrap_function(LIBODB, 'horizon2d_infos', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
    _linecount = wrap_function(LIBODB, 'horizon2d_linecount', ct.c_int, [ct.c_void_p])
    _lineids = wrap_function(LIBODB, 'horizon2d_lineids', None, [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_int)])
    _linenames = wrap_function(LIBODB, 'horizon2d_linenames', ct.c_void_p, [ct.c_void_p])
    _name = wrap_function(LIBODB, 'horizon2d_name', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _names = wrap_function(LIBODB, 'horizon2d_names', ct.c_void_p, [ct.c_void_p])

    def __init__(self, survey: Survey, horizon_name: str):
        self._horizon = Horizon2D._new(survey._survey, horizon_name.encode())

    def __del__(self):
        Horizon2D._del(self._horizon)

    @property
    def attribcount(self) ->int:
        """int: Number of attributes attached to this 2D horizon (readonly)"""
        return Horizon2D._attribcount(self._horizon)

    @property
    def attribnames(self) ->list[str]:
        """list[str]: Names of attributes attached to this 2D horizon (readonly)"""
        return pystrlist(Horizon2D._attribnames(self._horizon))

    @property
    def name(self) ->str:
        """str: Name of this 2D horizon (readonly)"""
        return pystr(Horizon2D._name(self._horizon))


    def info(self) ->dict:
        """Return basic information for this 2d horizon.
        
        Returns
        -------
        dict

        """

        return pyjsonstr(Horizon2D._info(self._horizon))

    def lineids(self) ->list[int]:
        """ Return the OpendTect line indices of 2D lines in this 2D horizon.

        Returns
        -------
        list[int]

        """

        ids = (ct.c_int * Horizon2D._linecount(self._horizon))()
        Horizon2D._lineids(self._horizon, len(ids), ids)
        return ids[:]

    def linenames(self) ->list[str]:
        """ Return the line names of 2D lines in this 2D horizon.

        Returns
        -------
        list[str]

        """

        return pystrlist(Horizon2D._linenames(self._horizon))

    @staticmethod
    def names(survey: Survey) ->list[str]:
        """ Return the names of 2d horizons in the given survey

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object

        Returns
        -------
        list[str]

        """

        return pystrlist(Horizon2D._names(survey._survey))

    @staticmethod
    def infos(survey: Survey, fornms: list=[]) ->dict:
        """ Return basic information for all or a subset of 2D horizons in the given survey.

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        fornms : list[str]
            A list of 2D horizon names to use, an empty list will give information for all 2D horizons.
            
        Returns
        -------
        dict

        """ 
        fornmsptr = stringset_new()
        for nm in fornms:
            stringset_add(fornmsptr, nm.encode())
        infolist = pyjsonstr(Horizon2D._infos(survey._survey, fornmsptr ))
        stringset_del(fornmsptr)
        return {key: [i[key] for i in infolist] for key in infolist[0]}



class Horizon3D(object):
    """
    A class for an OpendTect 3D horizon
    """

    _newin = wrap_function(LIBODB, 'horizon3d_newin', ct.c_void_p, [ct.c_void_p, ct.c_char_p])
    _newout = wrap_function(LIBODB, 'horizon3d_newout', ct.c_void_p, [ct.c_void_p, ct.c_char_p])
    _del = wrap_function(LIBODB, 'horizon3d_del', None, [ct.c_void_p])
    _attribcount = wrap_function(LIBODB, 'horizon3d_attribcount', ct.c_int, [ct.c_void_p])
    _attribnames = wrap_function(LIBODB, 'horizon3d_attribnames', ct.c_void_p, [ct.c_void_p])
    _feature = wrap_function(LIBODB, 'horizon3d_feature', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _features = wrap_function(LIBODB, 'horizon3d_features', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
    _info = wrap_function(LIBODB, 'horizon3d_info', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _infos = wrap_function(LIBODB, 'horizon3d_infos', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
    _name = wrap_function(LIBODB, 'horizon3d_name', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _names = wrap_function(LIBODB, 'horizon3d_names', ct.c_void_p, [ct.c_void_p])

    def __init__(self, survey: Survey, horizon_name: str, *args):
        """Initialise an OpendTect 3D Horizon object

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        horizon_name : str
            OpendTect 3D horizon name

        """

        self._horizon = Horizon3D._newin(survey._survey, horizon_name.encode())

    def __del__(self):
        Horizon3D._del(self._horizon)

    @property
    def attribcount(self) ->int:
        """int: Number of attributes attached to this 3D horizon (readonly)"""
        return Horizon3D._attribcount(self._horizon)

    @property
    def attribnames(self) ->list[str]:
        """list[str]: Names of attributes attached to this 3D horizon (readonly)"""
        return pystrlist(Horizon3D._attribnames(self._horizon))

    def info(self) ->dict:
        return pyjsonstr(Horizon3D._info(self._horizon))

    @property
    def name(self) ->str:
        return pystr(Horizon3D._name(self._horizon))

    @staticmethod
    def names(survey: Survey) ->list[str]:
        return pystrlist(Horizon3D._names(survey._survey))

    @staticmethod
    def infos(survey: Survey, fornms: list=[]) ->dict:
        """ Return basic information for all or a subset of 3D horizons in the given survey.

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        fornms : list[str]
            A list of 3D horizon names to use, an empty list will give information for all 3D horizons.
            
        Returns
        -------
        dict

        """ 
        fornmsptr = stringset_new()
        for nm in fornms:
            stringset_add(fornmsptr, nm.encode())
        infolist = pyjsonstr(Horizon3D._infos(survey._survey, fornmsptr ))
        stringset_del(fornmsptr)
        return {key: [i[key] for i in infolist] for key in infolist[0]}

    @staticmethod
    def features(survey: Survey, fornms: list=[]) ->str:
        """ Return a GeoJSON Feature Collection with the outlines and basic information for all 
        or a subset of 3d horizons in the given survey.

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        fornms : list[str]
            A list of 3D horizon names to use, an empty list will give information for all 3D horizons.
            
        Returns
        -------
        dict

        """ 
        fornmsptr = stringset_new()
        for nm in fornms:
            stringset_add(fornmsptr, nm.encode())
        res = pystr(Horizon3D._features(survey._survey, fornmsptr ))
        stringset_del(fornmsptr)
        return res

     
