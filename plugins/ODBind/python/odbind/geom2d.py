import numpy as np
import ctypes as ct
from collections import namedtuple
from collections.abc import Sequence
from enum import Enum
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del, unpack_slice, is_none_slice 
from odbind.survey import Survey, _SurveyObject

class Geom2D(_SurveyObject):
    """
    A class for an OpendTect 2D line geometry
    """

    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._newout = wrap_function(LIBODB, f'{bindnm}_newout', ct.c_void_p, [ct.c_void_p, ct.c_char_p, ct.c_bool])
        clss._close = wrap_function(LIBODB, f'{bindnm}_close', None, [ct.c_void_p])
        clss._getdata = wrap_function(LIBODB, f'{bindnm}_get', None, [ct.c_void_p, NumpyAllocator.CFUNCTYPE])
        putargs = [ct.c_void_p, ct.c_int,
                    np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.float32, ndim=1, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.float64, ndim=1, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.float64, ndim=1, flags="C_CONTIGUOUS")
                    ]
        clss._putdata = wrap_function(LIBODB, f'{bindnm}_put', None, putargs)


    def __init__(self, survey: Survey, name: str):
        """Initialise an OpendTect 2D line geometry object

        Parameters
        ----------
        survey : Survey
            an OpendTect survey object
        name : str
            an OpendTect 2D line name

        """
        super(Geom2D, self).__init__(survey, name)

    @classmethod
    def create(clss, survey: Survey, name: str, overwrite: bool=False):
        """Create a new OpendTect Geom2D object

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        name : str
            OpendTect Geom2D name
        overwrite : bool=False
            Flag to indicate if the new geometry can replace an existing geometry of the same name

        Returns
        -------
        A Geom2D object

        """

        newgeom = clss.__new__(clss)
        newgeom._survey = survey
        newgeom._handle = clss._newout(survey._handle, name.encode(), overwrite)
        if not newgeom._handle or not survey.isok:
            raise TypeError(survey.errmsg)
        elif not newgeom.isok:
            raise TypeError(newgeom.errmsg)

        return newgeom

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def close(self):
        self._close(self._handle)

    def getdata(self) ->dict:
        """Read the line geometry

        Returns a Python dict with the following keys and data:
        -  'line': str of the line name
        -  'trc': np.ndarray(int) with the trace numbers
        -  'ref': np.ndarray(float) with the SP numbers
        -  'x': np.ndarray(double) with the x coordinates of the traces  
        -  'y': np.ndarray(double) with the y coordinates of the traces

        Returns
        -------
        dict 

        """
        allocator = NumpyAllocator()

        self._getdata(self._handle, allocator.cfunc)
        if not self.isok:
            raise ValueError(self.errmsg)

        info =  {
                    'line': self.info()['name'],
                    'trc': allocator.allocated_arrays[0],
                    'ref': allocator.allocated_arrays[1],
                    'x': allocator.allocated_arrays[2],
                    'y': allocator.allocated_arrays[3],
                }
        return info

    def putdata(self, data):
        """Write the line geometry to an OpendTect

        Parameters
        ----------
        data : Python dict with the following keys and data:
                -  'trc': np.ndarray(int) with the trace numbers
                -  'ref': np.ndarray(float) with the SP numbers
                -  'x': np.ndarray(double) with the x coordinates of the traces  
                -  'y': np.ndarray(double) with the y coordinates of the traces

        """
        self._putdata(self._handle, len(data['trc']), data['trc'], data['ref'], data['x'], data['y'])
        if not self.isok:
            raise ValueError(self.errmsg)


Geom2D._initbindings('geom2d')

