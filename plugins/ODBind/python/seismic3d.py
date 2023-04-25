import numpy as np
import ctypes as ct
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del 
from odbind.survey import Survey, _SurveyObject

class Seismic3D(_SurveyObject):
    """
    A class for an OpendTect seismic volume
    """
    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._compnames = wrap_function(LIBODB, f'{bindnm}_compnames', ct.c_void_p, [ct.c_void_p])
        clss._nrbins = wrap_function(LIBODB, f'{bindnm}_nrbins', ct.c_int, [ct.c_void_p])
        clss._nrtrcs = wrap_function(LIBODB, f'{bindnm}_nrtrcs', ct.c_int, [ct.c_void_p])
        clss._gettrcidx = wrap_function(LIBODB, f'{bindnm}_gettrcidx', ct.c_int, [ct.c_void_p, ct.c_int, ct.c_int])
        clss._getinlcrl = wrap_function(LIBODB, f'{bindnm}_getinlcrl', None, [ct.c_void_p, ct.POINTER(ct.c_int), ct.POINTER(ct.c_int)])

    @property
    def comp_names(self) ->list[str]:
        """list[str]: Names of components in this seismic volume (readonly)"""
        return pystrlist(self._compnames(self._handle))

    @property
    def bin_count(self) ->int:
        """int : Number of bins within the extent of this seismic volume, number_of_bins>=number_of_traces"""
        return self._nrbins(self._handle)

    @property
    def trace_count(self) ->int:
        """int : Expected number of traces in this seismic volume."""
        return self._nrtrcs(self._handle)

    def trace_index(self, inline: int, crossline: int) ->int:
        """Return the trace index for a given inline, crossline location in this seismic volume.

        The first trace has an index of 0, the last trace has an index of number_of_traces-1.
        An index of -1 corresponds to an invalid inline,crline location.

        Parameters
        ----------
        inline : int
            the inline number.
        crossline : int
            the crossline number.
        
        Returns
        -------
        int : the corresponding trace number or -1 if the inline, crossline location is not within the seismic volume

        """
        return self._gettrcidx(self._handle, inline, crossline)

    def bin(self, trace_index: int) ->tuple[int]:
        """Return a tuple with the inline and crossline of the trace_index'th trace in this seismic volume

        Parameters
        ----------
        trace_index : int
            the trace index (first trace has an index of 0, last trace has an idex of number_of_traces-1)

        Returns
        -------
        tuple[int] : with the corresponding inline and crossline location

        """
        inline = ct.c_int
        crline = ct.c_int
        self._getinlcrl(self._handle, ct.byref(inline), ct.byref(crline))
        if not self._isok(self._handle):
            raise ValueError(seld._errmsg(self._handle))

        return (inline.value, crline.value)


Seismic3D._initbindings('seismic3d')
 
