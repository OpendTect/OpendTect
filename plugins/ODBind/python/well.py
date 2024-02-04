import numpy as np
import ctypes as ct
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del 
from odbind.survey import Survey, _SurveyObject

class Well(_SurveyObject):
    """
    A class for an OpendTect Well
    """
    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._lognames = wrap_function(LIBODB, f'{bindnm}_lognames', ct.c_void_p, [ct.c_void_p])
        clss._loginfo = wrap_function(LIBODB, f'{bindnm}_loginfo', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._markernames = wrap_function(LIBODB, f'{bindnm}_markernames', ct.c_void_p, [ct.c_void_p])
        clss._markerinfo = wrap_function(LIBODB, f'{bindnm}_markerinfo', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._track = wrap_function(LIBODB, f'{bindnm}_gettrack', None, [ct.c_void_p, NumpyAllocator.CFUNCTYPE])
        clss._logs = wrap_function(LIBODB, f'{bindnm}_getlogs', ct.POINTER(ct.c_char_p), [ct.c_void_p, NumpyAllocator.CFUNCTYPE, ct.c_void_p, ct.c_float, ct.c_bool])
        putlogargs = [  ct.c_void_p, ct.c_char_p,
                        np.ctypeslib.ndpointer(dtype=np.float32, ndim=1, flags="C_CONTIGUOUS"),
                        np.ctypeslib.ndpointer(dtype=np.float32, ndim=1, flags="C_CONTIGUOUS"),
                        ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_bool
                    ]
        clss._putlog = wrap_function(LIBODB, f'{bindnm}_putlog', None, putlogargs)
        clss._dellogs = wrap_function(LIBODB, f'{bindnm}_deletelogs', ct.c_bool, [ct.c_void_p, ct.c_void_p])
        clss._tvdss = wrap_function(LIBODB, f'{bindnm}_tvdss', None, [ct.c_void_p, ct.c_float, ct.POINTER(ct.c_float)])
        clss._tvd = wrap_function(LIBODB, f'{bindnm}_tvd', None, [ct.c_void_p, ct.c_float, ct.POINTER(ct.c_float)])
        clss._commonmarkernames = wrap_function(LIBODB, f'{bindnm}_commonmarkernames', ct.c_void_p, [ct.c_void_p, ct.c_void_p])
        clss._commonlognames = wrap_function(LIBODB, f'{bindnm}_commonlognames', ct.c_void_p, [ct.c_void_p, ct.c_void_p])

    @property
    def log_names(self) ->list[str]:
        """list[str]: Names of well logs in this well (readonly)"""
        return pystrlist(self._lognames(self._handle))

    @property
    def marker_names(self) ->list[str]:
        """list[str]: Names of well logs in this well (readonly)"""
        return pystrlist(self._markernames(self._handle))

    def log_info(self, forlognms: list[str]=[]) ->dict:
        """Return basic information for all or a subset of logs in this well.

        Parameters
        ----------
        forlognms : list[str]=[]
            (Optional) a list of log names to use. For an empty list information for all logs in the well is provided.
        
        Returns
        -------
        dict

        """
        fornmsptr = makestrlist(forlognms)
        infolist = pyjsonstr(self._loginfo(self._handle, fornmsptr ))
        stringset_del(fornmsptr)
        return infolist

    def log_info_dataframe(self, forlognms: list=[]) ->dict:
        """Return basic information for all or a subset of logs in this well as a Pandas DataFrame.
        
        Parameters
        ----------
        forlognms : list[str]=[]
            (Optional) a list of log names to use. For an empty list information for all logs in the well is provided.
        
        Returns
        -------
        Pandas Dataframe

        """
        from pandas import DataFrame
        infolist = self.log_info(forlognms)
        return DataFrame({key: [i[key] for i in infolist] for key in infolist[0]})

    def marker_info(self, formarkernms: list[str]=[]) ->dict:
        """Return basic information for all or a subset of markers in this well.
        
        Parameters
        ----------
        formarkernms : list[str]=[]
            (Optional) a list of marker names to use. For an empty list information for all markers in the well is provided.
        
        Returns
        -------
        dict

        """
        fornmsptr = makestrlist(formarkernms)
        infolist = pyjsonstr(self._markerinfo(self._handle, fornmsptr ))
        stringset_del(fornmsptr)
        return infolist

    def marker_info_dataframe(self, formarkernms: list[str]=[]) ->dict:
        """Return basic information for all or a subset of markers in this well as a Pandas DataFrame.
        
        Parameters
        ----------
        formarkernms : list[str]=[]
            (Optional) a list of marker names to use. For an empty list information for all markers in the well is provided.
        
        Returns
        -------
        Pandas Dataframe

        """
        from pandas import DataFrame
        infolist = self.marker_info(formarkernms)
        return DataFrame({key: [i[key] for i in infolist] for key in infolist[0]})

    def track(self):
        """Return dict of numpy arrays with the well track.

        Returns
        -------
        dict

        """
        allocator = NumpyAllocator()
        self._track(self._handle, allocator.cfunc)
        if not self._isok(self._handle):
            raise ValueError(pystr(self._errmsg(self._handle)))

        res =   {
                    'dah': allocator.allocated_arrays[0],
                    'tvdss': allocator.allocated_arrays[1],
                    'x': allocator.allocated_arrays[2],
                    'y': allocator.allocated_arrays[3]
                }
        return res;

    def track_dataframe(self):
        """Return the well track as a Pandas DataFrame .

        Returns
        -------
        Pandas DataFrame

        """
        from pandas import DataFrame
        return DataFrame(self.track()) 

    def logs(self, lognms: list[str]=[], zstep: float=0.5, upscale: bool=True):
        """Return dict of numpy arrays for all or a subset of logs in this well

        Parameters
        ----------
        lognms : list[str] = []
            list of log names or empty list for all
        zstep : float = 0.5
            (Optional) output sampling step in the surveys default depth unit
        upscale : bool = True
            (Optional) if True log is resampled by averaging over the step, otherwise use linear interpolation

        Returns
        -------
        dict[np.arrays] keyed by the log names, 'dah' is the depth log
        dict[str] of the log unit of measures, keyed by log name

        """
        lognmsptr = makestrlist(lognms)
        allocator = NumpyAllocator()
        uoms = pyjsonstr(self._logs(self._handle, allocator.cfunc, lognmsptr, zstep, upscale))
        stringset_del(lognmsptr)
        if not self.isok:
            raise ValueError(self.errmsg)

        logs = [key for key in uoms.keys()]
        data = { log: allocator.allocated_arrays[logs.index(log)] for log in logs }
        return data, uoms

    def logs_dataframe(self, lognms: list[str]=[], zstep: float=0.5, upscale: bool=True):
        """Return all or a subset of logs in this well as a Pandas DataFrame

        Parameters
        ----------
        lognms : list[str] = []
            list of log names or empty list for all
        zstep : float = 0.5
            (Optional) output sampling step in the surveys default depth unit
        upscale : bool = True
            (Optional) if True log is resampled by averaging over the step, otherwise use linear interpolation

        Returns
        -------
        Pandas DataFrame
        dict[str] of the log unit of measures, keyed by log name

        """
        from pandas import DataFrame, MultiIndex
        data, uoms = self.logs(lognms, zstep, upscale)
        df = DataFrame(data)
        return df, uoms

    def put_log(self, lognm: str, dah: np.ndarray, logdata: np.ndarray, uom: str=None, mnem: str=None, overwrite: bool=False):
        """Add a log curve to this well

        Parameters
        ----------
        lognm : str
            the name for the new log
        dah : np.ndarray
            1D float32 numpy array with the depth-along-hole (MD) of the logdata in the survey's default depth unit
        logdata : np.ndarray
            1D float32 numpy array with the log data
        uom : str
            the unit of measure of the new log
        mnem : str
            the Mnemonic of the new log
        overwrite : bool=False
            whether to overwrite if lognm already exists in the well

        """
        if dah.size!=logdata.size:
            raise ValueError("dah and logdata must be the same size")

        self._putlog(self._handle, lognm.encode(), dah, logdata, dah.size, uom.encode() if uom else None, 
                     mnem.encode() if mnem else None, overwrite)
        if not self.isok:
            raise ValueError(self.errmsg)

    def delete_logs(self, lognms: list[str]=[]):
        """Delete the listed log names from the well

        Parameters
        ----------
        lognms : list[str]
            list of log names to be deleted

        Returns
        -------
        bool : True/False indicating success/failure

        """
        lognmsptr = makestrlist(lognms)
        res = self._dellogs(self._handle, lognmsptr)
        stringset_del(lognmsptr)
        return res

    def tvdss(self, dah: float):
        """Return TVDSS for a MD/dah depth, all in the survey's default depth unit"""
        tvdss = ct.c_float()
        self._tvdss(self._handle, dah, ct.byref(tvdss))
        return tvdss.value

    def tvd(self, dah: float):
        """Return a TVD for a MD/dah depth, all in the survey's default depth unit"""
        tvd = ct.c_float()
        self._tvd(self._handle, dah, ct.byref(tvd))
        return tvd.value

    @staticmethod
    def common_markers(survey: Survey, forwells: list[str]=[] ):
        """Return list of markers common to the listed wells or all wells in the survey

        Parameters
        ----------
        survey : Survey
            the OpendTect survey object
        forwells : list[str]
            list of well names to consider

        Returns
        -------

        """
        wellnmsptr = makestrlist(forwells)
        res = pystrlist(self._commonmakernames(survey._handle, wellnmsptr))
        stringset_del(wellnmsptr)
        return res

Well._initbindings('well')
 
