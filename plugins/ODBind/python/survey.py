from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del 
import ctypes as ct

class Survey(object):
    """A class for an OpendTect survey/project.

    """
    _new = wrap_function(LIBODB, 'survey_new', ct.c_void_p, [ct.c_char_p, ct.c_char_p])
    _del = wrap_function(LIBODB, 'survey_del', None, [ct.c_void_p])
    _errmsg = wrap_function(LIBODB, 'survey_errmsg', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _isok = wrap_function(LIBODB, 'survey_isok', ct.c_bool, [ct.c_void_p])
    _bin = wrap_function(LIBODB, 'survey_bin', None, [ct.c_void_p, ct.c_double, ct.c_double, ct.POINTER(ct.c_int), ct.POINTER(ct.c_int)])
    _bincoords = wrap_function(LIBODB, 'survey_bincoords', None, [ct.c_void_p, ct.c_double, ct.c_double, ct.POINTER(ct.c_double), ct.POINTER(ct.c_double)])
    _coords = wrap_function(LIBODB, 'survey_coords', None, [ct.c_void_p, ct.c_int, ct.c_int, ct.POINTER(ct.c_double), ct.POINTER(ct.c_double)])
    _feature = wrap_function(LIBODB, 'survey_feature', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _features = wrap_function(LIBODB, 'survey_features', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_char_p])
    _has2d = wrap_function(LIBODB, 'survey_has2d', ct.c_bool, [ct.c_void_p])
    _has3d = wrap_function(LIBODB, 'survey_has3d', ct.c_bool, [ct.c_void_p])
    _hasobject = wrap_function(LIBODB, 'survey_hasobject', ct.c_bool, [ct.c_void_p, ct.c_char_p, ct.c_char_p])
    _info = wrap_function(LIBODB, 'survey_info', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _infos = wrap_function(LIBODB, 'survey_infos', ct.POINTER(ct.c_char_p), [ct.c_void_p,ct.c_char_p])
    _names = wrap_function(LIBODB, 'survey_names', ct.c_void_p, [ct.c_char_p])
    _path = wrap_function(LIBODB, 'survey_path', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _type = wrap_function(LIBODB, 'survey_survtype', ct.POINTER(ct.c_char_p), [ct.c_void_p])
    _zrange = wrap_function(LIBODB, 'survey_zrange', None, [ct.c_void_p, ct.POINTER(ct.c_float)])

    def __init__(self, survey_name: str, basedir: str=None):
        """Initialise an OpendTect survey object

        Parameters
        ----------
        survey_name : str
            OpendTect survey name
        basedir : str = None
            (Optional)OpendTect data directory/folder, defaults to location set in user's OpendTect settings

        """
        self._handle = Survey._new(survey_name.encode(), basedir.encode() if basedir else None)
        if not self.isok:
            raise ValueError(self.errmsg)

    def __del__(self):
        Survey._del(self._handle)

    @property
    def handle(self) ->ct.c_void_p:
        """ Return the ctypes pointer to the underlying C/C++ Survey object ."""
        return self._handle

    @property
    def isok(self) ->bool:
        """ Return the current status of the underlying C/C++ Survey object."""
        return self._isok(self._handle)

    @property
    def errmsg(self) ->str:
        """ Return the current error message string of the underlying C/C++ Survey object."""
        return pystr(self._errmsg(self._handle))

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

    @property
    def zrange(self) ->list[float]:
        """list[float]: Z range in survey definition (readonly)"""
        zrg = [0.0, 0.0, 0.0]
        ct_zrg = (ct.c_float * 3)(*zrg)

        Survey._zrange(self._handle, ct_zrg)
        return list(ct_zrg)

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
        tuple[int, int] : The nearest inline and crossline location to the given X and Y coordinates

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
        tuple[float, float] : The decimal inline and crossline location equivalent to the given X and Y coordinates

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
        tuple[float, float] : The X and Y coordinates equivalent to the given inline and crossline

        """
        x = ct.c_double()
        y = ct.c_double()
        Survey._coords(self._handle, iline, xline, ct.byref(x), ct.byref(y))
        return (x.value, y.value)

    def feature(self) ->dict:
        """ Return a GeoJSON feature for the survey

        """
        return '{"type":"FeatureCollection","features":['+ pystr(Survey._feature(self._handle)) + ']}'

    def info(self) ->dict:
        """Return basic information for the OpendTect survey.
        
        Returns
        -------
        dict

        """
        return pyjsonstr(Survey._info(self._handle))

    @staticmethod
    def names(basedir: str=None) ->list[str]:
        """ Return the names of all surveys

        Parameters
        ----------
        basedir : str=None
            (Optional) an OpendTect data directory/folder to use, defaults to location set in user's OpendTect settings

        Returns
        -------
        list[str]

        """
        return pystrlist(Survey._names(basedir.encode() if basedir else None))

    @staticmethod
    def infos(fornms: list=[], basedir: str=None) ->dict:
        """ Return basic information for all or a subset of surveys

        Parameters
        ----------
        fornms : list[str]=[]
            (Optional) a list of survey names to use. For an empty list information for all surveys is provided.
        basedir : str=None
            (Optional) an OpendTect data directory/folder to use, defaults to location set in user's OpendTect settings
            
        Returns
        -------
        dict

        """ 
        fornmsptr = makestrlist(fornms)
        infolist = pyjsonstr(Survey._infos(fornmsptr, basedir.encode() if basedir else None))
        stringset_del(fornmsptr)
        return infolist

    @staticmethod
    def infos_dataframe(fornms: list=[], basedir: str=None):
        """ Return basic information for all or a subset of surveys as a Pandas DataFrame.

        Parameters
        ----------
        fornms : list[str]=[]
            (Optional) a list of survey names to use. For an empty list information for all surveys is provided.
        basedir : str=None
            (Optional) an OpendTect data directory/folder to use, defaults to location set in user's OpendTect settings
            
        Returns
        -------
        Pandas DataFrame

        """
        from pandas import DataFrame
        infolist = Survey.infos(fornms, basedir)
        return DataFrame({key: [i[key] for i in infolist] for key in infolist[0]})


    @staticmethod
    def features(fornms: list=[], basedir: str=None) ->str:
        """ Return a GeoJSON Feature Collection with the outlines and basic information for all 
        or a subset of surveys.

        Parameters
        ----------
        fornms : list[str]=[]
            (Optional) a list of survey names to use. For an empty list information for all surveys is provided.
        basedir : str=None
            (Optional) an OpendTect data directory/folder to use, defaults to location set in user's OpendTect settings
            
        Returns
        -------
        dict

        """ 
        fornmsptr = makestrlist(fornms)
        res = pystr(Survey._features(fornmsptr, basedir.encode() if basedir else None ))
        stringset_del(fornmsptr)
        return res

import odbind as odb
import ctypes as ct

class _SurveyObject(object):
    """
    Base class for OpendTect survey objects - not useful on its own
    """

    @classmethod
    def _initbasebindings(clss, bindnm):
        clss._newin = odb.wrap_function(LIBODB, f'{bindnm}_newin', ct.c_void_p, [ct.c_void_p, ct.c_char_p])
        clss._del = odb.wrap_function(LIBODB, f'{bindnm}_del', None, [ct.c_void_p])
        clss._errmsg = odb.wrap_function(LIBODB, f'{bindnm}_errmsg', ct.POINTER(ct.c_char_p), [ct.c_void_p])
        clss._feature = odb.wrap_function(LIBODB, f'{bindnm}_feature', ct.POINTER(ct.c_char_p), [ct.c_void_p])
        clss._features = odb.wrap_function(LIBODB, f'{bindnm}_features', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._info = odb.wrap_function(LIBODB, f'{bindnm}_info', ct.POINTER(ct.c_char_p), [ct.c_void_p])
        clss._infos = odb.wrap_function(LIBODB, f'{bindnm}_infos', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._isok = odb.wrap_function(LIBODB, f'{bindnm}_isok', ct.c_bool, [ct.c_void_p])
        clss._names = odb.wrap_function(LIBODB, f'{bindnm}_names', ct.c_void_p, [ct.c_void_p])
        clss._removeobjs = odb.wrap_function(LIBODB, f'{bindnm}_removeobjs', None, [ct.c_void_p, ct.c_void_p])

    def __init__(self, survey: Survey, name: str):
        """Initialise an OpendTect object

        Parameters
        ----------
        survey : Survey
            an OpendTect survey object
        name : str
            an OpendTect object name

        """
        self._survey = survey
        self._handle = self._newin( survey._handle, name.encode())
        if not self.isok:
            raise TypeError(self.errmsg)

    @property
    def survey(self) ->Survey: 
        """ Return the Survey object that this item is from."""
        return self._survey

    @property
    def handle(self) ->ct.c_void_p:
        """ Return the ctypes pointer to the underlying C/C++ object ."""
        return self._handle

    @property
    def isok(self) ->bool:
        """ Return the current status of the underlying C/C++ object."""
        return self._isok(self._handle)

    @property
    def errmsg(self) ->str:
        """ Return the current error message string of the underlying C/C++ object."""
        return pystr(self._errmsg(self._handle))

    def __del__(self):
        self._del(self._handle)

    def feature(self) ->dict:
        """ Return a GeoJSON feature for the object

        """
        return '{"type":"FeatureCollection","features":['+ pystr(self._feature(self._handle)) + ']}'

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
        fornmsptr = makestrlist(fornms)
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
        fornmsptr = makestrlist(fornms)
        infolist = pyjsonstr(clss._infos(survey._handle, fornmsptr ))
        stringset_del(fornmsptr)
        return infolist

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
        infolist =  clss.infos(survey, fornms)
        return DataFrame({key: [i[key] for i in infolist] for key in infolist[0]})

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

    @classmethod
    def delete(clss, survey: Survey, nms: list[str]):
        """Removes the listed objects from the given survey.

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        nms : list[str]
            A list of object names to remove.

        """

        nmsptr = makestrlist(nms)
        clss._removeobjs(survey._handle, nmsptr)
        stringset_del(nmsptr)
        if not survey.isok:
            raise OSError(survey.errmsg) 





