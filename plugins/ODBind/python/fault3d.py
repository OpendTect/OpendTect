import numpy as np
import ctypes as ct
from collections import namedtuple
from collections.abc import Sequence
from enum import Enum
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del, unpack_slice, is_none_slice 
from odbind.survey import Survey, _SurveyObject

class Fault3D(_SurveyObject):
    """
    A class for an OpendTect Fault3D
    """

    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._getstick = wrap_function(LIBODB, f'{bindnm}_getstick', None, [ct.c_void_p, ct.c_int, NumpyAllocator.CFUNCTYPE])
        clss._getstickcount = wrap_function(LIBODB, f'{bindnm}_stickcount', ct.c_int, [ct.c_void_p])


    def __init__(self, survey: Survey, name: str):
        """Initialise an OpendTect Fault3D

        Parameters
        ----------
        survey : Survey
            an OpendTect survey object
        name : str
            an OpendTect Fault3D name

        """
        super(Fault3D, self).__init__(survey, name)

    def __len__(self):
        """Number of fault sticks in fault"""
        return self._getstickcount(self._handle)

    def __getitem__(self, idx):
        """[number] return the fault stick for given index.

        [number] returns a single fault stick,.
        
        """
        if isinstance(idx, int):
            idx = self.wrapindex(idx)
            return self.getstick(idx) 
        else:
            raise TypeError('index should be an int')

    def getstick(self, idx):
        allocator = NumpyAllocator()
        self._getstick(self._handle, idx, allocator.cfunc)
        if not self.isok:
            raise ValueError(self.errmsg)
        info =  {
                    'x': allocator.allocated_arrays[0],
                    'y': allocator.allocated_arrays[1],
                    'z': allocator.allocated_arrays[2],
                }
        return info



Fault3D._initbindings('fault3d')

