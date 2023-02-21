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
import numpy as np
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

class NumpyAllocator:
    CFUNCTYPE = ct.CFUNCTYPE(ct.c_long, ct.c_int, ct.POINTER(ct.c_int), ct.c_char)

    def __init__(self):
        self.allocated_arrays = []

    def __call__(self, dims, shape, dtype):
        x = np.empty(shape[:dims], np.dtype(dtype))
        self.allocated_arrays.append(x)
        return x.ctypes.data_as(ct.c_void_p).value

    def getcfunc(self):
        return self.CFUNCTYPE(self)
    cfunc = property(getcfunc)

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
    _feature = wrap_function(LIBODB, 'survey_feature', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _features = wrap_function(LIBODB, 'survey_features', ct.POINTER(ct.c_char_p), [ct.c_char_p, ct.c_void_p])
    _has2d = wrap_function(LIBODB, 'survey_has2d', ct.c_bool, [ct.c_void_p])
    _has3d = wrap_function(LIBODB, 'survey_has3d', ct.c_bool, [ct.c_void_p])
    _hasobject = wrap_function(LIBODB, 'survey_hasobject', ct.c_bool, [ct.c_void_p, ct.c_char_p, ct.c_char_p])
    _info = wrap_function(LIBODB, 'survey_info', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _infos = wrap_function(LIBODB, 'survey_infos', ct.POINTER(ct.c_char_p), [ct.c_char_p, ct.c_void_p])
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
        self._handle = Survey._new(basedir.encode(), survey_name.encode())

    def __del__(self):
        Survey._del(self._handle)

    @property
    def has2d(self) ->bool:
        """bool: True if the survey contains 2D data, False otherwise (readonly)"""
        return Survey._has2d(self._handle)

    @property
    def has3d(self) ->bool:
        """bool: True if the survey contains 3D data, False otherwise (readonly)"""
        return Survey._has3d(self._handle)

    @property
    def survey_type(self) ->str:
        """str: Survey type: one of 2D, 3D or 2D3D (readonly)"""
        return pystr(Survey._type(self._handle))

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
        Survey._bin(self._handle, x, y, ct.byref(iline), ct.byref(xline))
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
        Survey._bincoords(self._handle, x, y, ct.byref(iline), ct.byref(xline))
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
        Survey._coords(self._handle, iline, xline, ct.byref(x), ct.byref(y))
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
        return pyjsonstr(Survey._info(self._handle))

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


class _SurveyObject(object):
    """
    Base class for OpendTect survey objects - not useful on its own
    """

    @classmethod
    def _initbasebindings(clss, bindnm):
        clss._newin = wrap_function(LIBODB, f'{bindnm}_newin', ct.c_void_p, [ct.c_void_p, ct.c_char_p])
        clss._del = wrap_function(LIBODB, f'{bindnm}_del', None, [ct.c_void_p])
        clss._errmsg = wrap_function(LIBODB, f'{bindnm}_feature', ct.POINTER(ct.c_char_p), [ct.c_void_p])
        clss._feature = wrap_function(LIBODB, f'{bindnm}_feature', ct.POINTER(ct.c_char_p), [ct.c_void_p])
        clss._features = wrap_function(LIBODB, f'{bindnm}_features', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._info = wrap_function(LIBODB, f'{bindnm}_info', ct.POINTER(ct.c_char_p), [ct.c_void_p])
        clss._infos = wrap_function(LIBODB, f'{bindnm}_infos', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._isok = wrap_function(LIBODB, f'{bindnm}_isok', ct.c_bool, [ct.c_void_p])
        clss._names = wrap_function(LIBODB, f'{bindnm}_names', ct.c_void_p, [ct.c_void_p])

    def __init__(self, survey: Survey, name: str):
        """Initialise an OpendTect object

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        name : str
            OpendTect object name

        """

        if not hasattr(self,'_newin'):
            self._initbindings(type(self).__name__.lower())

        self._survey = survey
        self._handle = self._newin( survey._handle, name.encode())
        if not self._isok(self._handle):
            raise TypeError(self._errmsg(self._handle))

    def __del__(self):
        self._del(self._handle)

    def feature(self) ->dict:
        """ Return a GeoJSON feature for the object

        """
        return pyjsonstr(self._feature(self._handle))

    @classmethod
    def features(clss, survey: Survey, fornms: list=[]) ->str:
        """ Return a GeoJSON Feature Collection for all or a subset of objects in the given survey.

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        fornms : list[str]
            A list of object names to use, an empty list will give information for all objects.            

        Returns
        -------
        dict

        """ 
        fornmsptr = stringset_new()
        for nm in fornms:
            stringset_add(fornmsptr, nm.encode())
        res = pystr(clss._features(survey._handle, fornmsptr ))
        stringset_del(fornmsptr)
        return res

    def info(self) ->dict:
        """Return basic information for this object.
        
        Returns
        -------
        dict

        """

        return pyjsonstr(self._info(self._handle))

    @classmethod
    def infos(clss, survey: Survey, fornms: list=[]) ->dict:
        """ Return basic information for all or a subset of objects in the given survey.

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        fornms : list[str]
            A list of object names to use, an empty list will give information for all objects.
            
        Returns
        -------
        dict

        """ 
        fornmsptr = stringset_new()
        for nm in fornms:
            stringset_add(fornmsptr, nm.encode())
        infolist = pyjsonstr(clss._infos(survey._handle, fornmsptr ))
        stringset_del(fornmsptr)
        return {key: [i[key] for i in infolist] for key in infolist[0]}

    @classmethod
    def infos_dataframe(clss, survey: Survey, fornms: list=[]) ->dict:
        """ Return basic information for all or a subset of objects in the given survey as a Pandas DataFrame.

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        fornms : list[str]
            A list of object names to use, an empty list will give information for all objects.
            
        Returns
        -------
        dict

        """
        from pandas import DataFrame
        return DataFrame(clss.infos(survey, fornms))

    @classmethod
    def names(clss, survey: Survey) ->list[str]:
        """ Return the names of this object type in the given survey

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object

        Returns
        -------
        list[str]

        """

        return pystrlist(clss._names(survey._handle))




class Horizon2D(_SurveyObject):
    """
    A class for an OpendTect 2D horizon
    """

    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._attribcount = wrap_function(LIBODB, f'{bindnm}_attribcount', ct.c_int, [ct.c_void_p])
        clss._attribnames = wrap_function(LIBODB, f'{bindnm}_attribnames', ct.c_void_p, [ct.c_void_p])
        clss._linecount = wrap_function(LIBODB, f'{bindnm}_linecount', ct.c_int, [ct.c_void_p])
        clss._lineids = wrap_function(LIBODB, f'{bindnm}_lineids', None, [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_int)])
        clss._linenames = wrap_function(LIBODB, f'{bindnm}_linenames', ct.c_void_p, [ct.c_void_p])

    @property
    def attribcount(self) ->int:
        """int: Number of attributes attached to this 2D horizon (readonly)"""
        return self._attribcount(self._handle)

    @property
    def attribnames(self) ->list[str]:
        """list[str]: Names of attributes attached to this 2D horizon (readonly)"""
        return pystrlist(self._attribnames(self._handle))

    def lineids(self) ->list[int]:
        """ Return the OpendTect line indices of 2D lines in this 2D horizon.

        Returns
        -------
        list[int]

        """

        ids = (ct.c_int * self._linecount(self._handle))()
        self._lineids(self._handle, len(ids), ids)
        return ids[:]

    def linenames(self) ->list[str]:
        """ Return the line names of 2D lines in this 2D horizon.

        Returns
        -------
        list[str]

        """

        return pystrlist(self._linenames(self._handle))


class Horizon3D(_SurveyObject):
    """
    A class for an OpendTect 3D horizon
    """
    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._newout = wrap_function(LIBODB, f'{bindnm}_newout', ct.c_void_p, [ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_int), ct.POINTER(ct.c_int), ct.c_bool])
        clss._attribnames = wrap_function(LIBODB, f'{bindnm}_attribnames', ct.c_void_p, [ct.c_void_p])
        clss._getz = wrap_function(LIBODB, f'{bindnm}_getz', ct.c_void_p, [ct.c_void_p, NumpyAllocator.CFUNCTYPE])
        clss._getxy = wrap_function(LIBODB, f'{bindnm}_getxy', ct.c_void_p, [ct.c_void_p, NumpyAllocator.CFUNCTYPE])
        putzargs = [ct.c_void_p, 
                    np.ctypeslib.ndpointer(dtype=np.uint32, ndim=1, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.float32, ndim=2, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags="C_CONTIGUOUS")
                    ]
        clss._putz = wrap_function(LIBODB, f'{bindnm}_getz', None, putzargs)

    @property
    def attribnames(self) ->list[str]:
        """list[str]: Names of attributes attached to this 3D horizon (readonly)"""
        return pystrlist(self._attribnames(self._handle))

    @property
    def ilines(self) ->list[int]:
        """list[int]: Inline numbers included in this 3D horizon (readonly)"""
        hi_inl = self.info()['inl_range']
        inlrg = range(hi_inl[0], hi_inl[1]+hi_inl[2], hi_inl[2] )
        return [inl for inl in inlrg]

    @property
    def xlines(self) ->list[int]:
        """list[int]: Crossline numbers included in this 3D horizon (readonly)"""
        hi_crl = self.info()['crl_range']
        crlrg = range(hi_crl[0], hi_crl[1]+hi_crl[2], hi_crl[2] )
        return [crl for crl in crlrg]

    @classmethod
    def create(clss, survey: Survey, name: str, inl_rg: list[int], crl_rg: list[int], overwrite: bool=False):
        if not hasattr(clss,'_newin'):
            clss._initbindings(type(clss).__name__.lower())

        newhor = clss.__new__(clss)
        newhor._survey = survey
        ct_inlrg = (ct.c_int * 3)(*inl_rg)
        ct_crlrg = (ct.c_int * 3)(*crl_rg)
        newhor._handle = clss._newout(survey._handle, name, ct_inl_rg, ct_crl_rg, overwrite)
        if not clss._isok(newhor._handle):
            raise TypeError(clss._errmsg(newhor._handle))

        return newhor

    def getz(self):
        allocator = NumpyAllocator()
        self._getz(self._handle, allocator.cfunc)
        if not self._isok(self._handle):
            raise ValueError(self._errmsg(self._handle))

        return allocator.allocated_arrays[0]
     
    def getxy(self):
        allocator = NumpyAllocator()
        self._getxy(self._handle, allocator.cfunc)
        if not self._isok(self._handle):
            raise ValueError(self._errmsg(self._handle))

        return (allocator.allocated_arrays[:2])

    def get_xarray(self):
        from xarray import DataArray
        name = self.info()['name']
        xy = self.getxy()
        z = self.getz()
        si = self._survey.info()
        dims = ['inl', 'crl']
        xyattrs = { 'units': si['xyunit']}
        coords =    {
                        'inl': self.ilines,
                        'crl': self.xlines,
                        'x': DataArray(xy[0], dims=dims, attrs=xyattrs),
                        'y': DataArray(xy[1], dims=dims, attrs=xyattrs)
                    }
        attribs =   {
                        'description': name,
                        'units': si['zunit'],
                        'crs': si['crs']
                    }
        return DataArray(z, coords=coords, dims=dims, name=name, attrs=attribs)

    def putz(self, data, inlines, crlines):
        npdata = data if data.isinstance('ndarray') else np.array(data)
        npinlines = inlines if inlines.isinstance('ndarray') else np.array(inlines)
        npcrlines = crlines if crlines.isinstance('ndarray') else np.array(crlines)
        shape = np.array(npdata.shape, dtype=np.uint32)
        self._putz(self._handle, shape, npdata, npinlines, npcrlines)
        if not self._isok(self._handle):
            raise ValueError(self._errmsg(self._handle))


