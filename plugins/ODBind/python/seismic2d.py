import numpy as np
import ctypes as ct
from collections import namedtuple
from collections.abc import Sequence
from enum import Enum
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del, unpack_slice, is_none_slice 
from odbind.survey import Survey, _SurveyObject

class Seismic2D(_SurveyObject):
    """
    A class for an OpendTect 2D seismic dataset
    """

    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._close = wrap_function(LIBODB, f'{bindnm}_close', None, [ct.c_void_p])
        clss._linenames = wrap_function(LIBODB, f'{bindnm}_linenames', ct.c_void_p, [ct.c_void_p])
        clss._lineinfo = wrap_function(LIBODB, f'{bindnm}_lineinfo', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])

    def __init__(self, survey: Survey, name: str):
        """Initialise an OpendTect 2D seismic dataset object

        Parameters
        ----------
        survey : Survey
            an OpendTect survey object
        name : str
            an OpendTect 2D seismic dataset name

        """
        super(Seismic2D, self).__init__(survey, name)

    @property
    def line_names(self) ->list[str]:
        """list[str]: Names of lines in this seismic dataset (readonly)"""
        return pystrlist(self._linenames(self._handle))

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def close(self):
        self._close(self._handle)

    def line_info(self, forlinenms: list[str]=[]) ->dict:
        """Return basic information for all or a subset of lines in this 2D dataset.
        
        Parameters
        ----------
        forlinenms : list[str]=[]
            (Optional) a list of line names to use. For an empty list information for all lines in the dataset is provided.
        
        Returns
        -------
        dict

        """
        fornmsptr = makestrlist(forlinenms)
        infolist = pyjsonstr(self._lineinfo(self._handle, fornmsptr ))
        stringset_del(fornmsptr)
        return infolist

    def line_info_dataframe(self, forlinenms: list[str]=[]) ->dict:
        """Return basic information for all or a subset of lines in this 2D dataset as a Pandas DataFrame.
        
        Parameters
        ----------
        forlinenms : list[str]=[]
            (Optional) a list of line names to use. For an empty list information for all lines in the 2D dataset is provided.
        
        Returns
        -------
        Pandas Dataframe

        """
        from pandas import DataFrame
        infolist = self.line_info(forlinenms)
        return DataFrame({key: [i[key] for i in infolist] for key in infolist[0]})


Seismic2D._initbindings('seismic2d')

