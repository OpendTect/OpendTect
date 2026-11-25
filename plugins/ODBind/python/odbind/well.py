import re
import numpy as np
import ctypes as ct
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del 
from odbind.survey import Survey, _SurveyObject

_COLOR_RE = re.compile(r"^#[0-9a-fA-F]{6}$")

class Well(_SurveyObject):
    """
    A class for an OpendTect Well
    """
    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._newout = wrap_function(LIBODB, f'{bindnm}_newout', ct.c_void_p,
                                     [ct.c_void_p, ct.c_char_p, ct.c_double,
                                      ct.c_double, ct.c_float, ct.c_float,
                                      ct.c_bool])
        clss._lognames = wrap_function(LIBODB, f'{bindnm}_lognames', ct.c_void_p, [ct.c_void_p])
        clss._loginfo = wrap_function(LIBODB, f'{bindnm}_loginfo', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._markernames = wrap_function(LIBODB, f'{bindnm}_markernames', ct.c_void_p, [ct.c_void_p])
        clss._markerinfo = wrap_function(LIBODB, f'{bindnm}_markerinfo', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._track = wrap_function(LIBODB, f'{bindnm}_gettrack', None, [ct.c_void_p, NumpyAllocator.CFUNCTYPE])
        clss._logs = wrap_function(LIBODB, f'{bindnm}_getlogs', ct.POINTER(ct.c_char_p), [ct.c_void_p, NumpyAllocator.CFUNCTYPE, ct.c_void_p, ct.c_float, ct.c_bool])
        clss._log = wrap_function(LIBODB, f'{bindnm}_getlog', ct.POINTER(ct.c_char_p), [ct.c_void_p, NumpyAllocator.CFUNCTYPE, ct.c_char_p])
        putlogargs = [  ct.c_void_p, ct.c_char_p,
                        np.ctypeslib.ndpointer(dtype=np.float32, ndim=1, flags="C_CONTIGUOUS"),
                        np.ctypeslib.ndpointer(dtype=np.float32, ndim=1, flags="C_CONTIGUOUS"),
                        ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_bool
                    ]
        clss._putlog = wrap_function(LIBODB, f'{bindnm}_putlog', None, putlogargs)
        clss._dellogs = wrap_function(LIBODB, f'{bindnm}_deletelogs', ct.c_bool, [ct.c_void_p, ct.c_void_p])
        putmarkersargs = [  ct.c_void_p, ct.c_void_p,
                            np.ctypeslib.ndpointer(dtype=np.float32, ndim=1, flags="C_CONTIGUOUS"),
                            ct.c_void_p, ct.c_bool
                        ]
        clss._putmarkers = wrap_function(LIBODB, f'{bindnm}_putmarkers', None, putmarkersargs)
        clss._delmarkers = wrap_function(LIBODB, f'{bindnm}_deletemarkers', ct.c_bool, [ct.c_void_p, ct.c_void_p])
        puttrackargs = [    ct.c_void_p,
                            np.ctypeslib.ndpointer(dtype=np.float32, ndim=1, flags="C_CONTIGUOUS"),
                            np.ctypeslib.ndpointer(dtype=np.float32, ndim=1, flags="C_CONTIGUOUS"),
                            np.ctypeslib.ndpointer(dtype=np.float64, ndim=1, flags="C_CONTIGUOUS"),
                            np.ctypeslib.ndpointer(dtype=np.float64, ndim=1, flags="C_CONTIGUOUS"),
                            ct.c_int
                        ]
        clss._puttrack = wrap_function(LIBODB, f'{bindnm}_puttrack', None, puttrackargs)
        clss._tvdss = wrap_function(LIBODB, f'{bindnm}_tvdss', None, [ct.c_void_p, ct.c_float, ct.POINTER(ct.c_float)])
        clss._tvd = wrap_function(LIBODB, f'{bindnm}_tvd', None, [ct.c_void_p, ct.c_float, ct.POINTER(ct.c_float)])
        clss._commonmarkernames = wrap_function(LIBODB, f'{bindnm}_commonmarkernames', ct.c_void_p, [ct.c_void_p, ct.c_void_p])
        clss._commonlognames = wrap_function(LIBODB, f'{bindnm}_commonlognames', ct.c_void_p, [ct.c_void_p, ct.c_void_p])

    @classmethod
    def create(clss, survey: Survey, name: str, surfacecoord_x: float,
               surfacecoord_y: float, kbelev: float, td: float,
               overwrite: bool=False):
        """Create a new OpendTect well object

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        name : str
            OpendTect well name
        surfacecoord_x : float
            X coordinate of the well surface location
        surfacecoord_y : float
            Y coordinate of the well surface location
        kbelev : float
            Elevation of the Kelly Bushing, in the survey's default depth unit
        td : float
            Total depth (MD) of the well, in the survey's default depth unit
        overwrite : bool=False
            Flag to indicate if the new well can replace an existing well
            of the same name

        Returns
        -------
        A Well object with a simple vertical track from KB to TD

        """

        newwell = clss.__new__(clss)
        newwell._survey = survey
        newwell._handle = clss._newout(survey._handle, name.encode(),
                                       surfacecoord_x, surfacecoord_y,
                                       kbelev, td, overwrite)
        if not newwell._handle or not survey.isok:
            raise TypeError(survey.errmsg)
        elif not newwell.isok:
            raise TypeError(newwell.errmsg)

        return newwell

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        pass

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

    def log(self, lognm: str):
        """Return a single log in this well as stored, without resampling

        Parameters
        ----------
        lognm : str
            the name of the log

        Returns
        -------
        dict[np.arrays] with the stored depths keyed by 'dah' and the stored values keyed by the log name
        dict[str] of the log unit of measures, keyed by 'dah' and the log name

        """
        allocator = NumpyAllocator()
        uoms = pyjsonstr(self._log(self._handle, allocator.cfunc, lognm.encode()))
        if not self.isok:
            raise ValueError(self.errmsg)

        logs = [key for key in uoms.keys()]
        data = { log: allocator.allocated_arrays[logs.index(log)] for log in logs }
        return data, uoms

    def log_dataframe(self, lognm: str):
        """Return a single log in this well as stored, without resampling, as a Pandas DataFrame

        Parameters
        ----------
        lognm : str
            the name of the log

        Returns
        -------
        Pandas DataFrame
        dict[str] of the log unit of measures, keyed by 'dah' and the log name

        """
        from pandas import DataFrame
        data, uoms = self.log(lognm)
        return DataFrame(data), uoms

    def put_log(self, lognm: str, dah: np.ndarray, logdata: np.ndarray, uom: str=None, mnem: str=None, overwrite: bool=False):
        """Add a log curve to this well

        Parameters
        ----------
        lognm : str
            the name for the new log
        dah : np.ndarray
            1D float32 numpy array with the depth-along-hole (MD) of the logdata in the survey's default depth unit
        logdata : np.ndarray
            1D float32 numpy array with the log data, NaN for undefined values. Leading and trailing
            undefined values are not stored, so once reloaded the log starts and ends at a defined value
        uom : str
            the unit of measure of the new log, a unit OpendTect does not recognise is not stored
            and the log has no unit of measure
        mnem : str
            the Mnemonic of the new log
        overwrite : bool=False
            whether to overwrite if lognm already exists in the well. If False and lognm exists, nothing
            is written and no error is raised. If True the existing log is deleted before the new log is written

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

    def put_markers(self, markernms: list[str], dah: np.ndarray, colors: list[str]=[], overwrite: bool=False):
        """Add markers to this well

        Parameters
        ----------
        markernms : list[str]
            the names of the new markers
        dah : np.ndarray
            1D float32 numpy array with the depth-along-hole (MD) of each marker in the survey's default depth unit
        colors : list[str]=[]
            (Optional) the color of each marker as a '#rrggbb' string, the format marker_info returns.
            For an empty list the markers get OpendTect's default color (white)
        overwrite : bool=False
            whether to overwrite markers that already exist in the well. If False an existing marker is
            left as it is and no error is raised. If True the existing marker is replaced by the new one

        """
        if dah.size!=len(markernms):
            raise ValueError("markernms and dah must be the same size")
        if colors and len(colors)!=len(markernms):
            raise ValueError("colors must be empty or the same size as markernms")
        for i, color in enumerate(colors):
            if not isinstance(color, str) or not _COLOR_RE.fullmatch(color):
                raise ValueError(f"colors[{i}] must be a '#rrggbb' hex color string, got {color!r}")

        markernmsptr = makestrlist(markernms)
        colorsptr = makestrlist([color.lower() for color in colors])
        self._putmarkers(self._handle, markernmsptr, dah, colorsptr, overwrite)
        stringset_del(markernmsptr)
        stringset_del(colorsptr)
        if not self.isok:
            raise ValueError(self.errmsg)

    def delete_markers(self, markernms: list[str]=[]):
        """Delete the listed markers from the well

        Parameters
        ----------
        markernms : list[str]
            list of marker names to be deleted

        Returns
        -------
        bool : True/False indicating success/failure

        """
        markernmsptr = makestrlist(markernms)
        res = self._delmarkers(self._handle, markernmsptr)
        stringset_del(markernmsptr)
        return res

    def put_track(self, dah: np.ndarray, tvdss: np.ndarray, x: np.ndarray, y: np.ndarray):
        """Replace the track of this well

        The arguments are the arrays track() returns, so well.put_track(**track) writes such a dict back.
        The KB elevation (dah - tvdss at the first point), the TD (last dah) and the surface coordinate
        (first x, y) in info() follow the new track. The active time-depth model is projected onto the new
        track: it keeps the TWT at each TVDSS and is extended to the new TD. Logs, markers and the checkshot
        model are stored by dah and are kept as they are, as OpendTect's own track editor does

        Parameters
        ----------
        dah : np.ndarray
            1D float32 numpy array with the depth-along-hole (MD) of each track point in the survey's default
            depth unit, at least two points and increasing
        tvdss : np.ndarray
            1D float32 numpy array with the TVDSS of each track point in the survey's default depth unit
        x : np.ndarray
            1D float64 numpy array with the X coordinate of each track point
        y : np.ndarray
            1D float64 numpy array with the Y coordinate of each track point

        """
        if not dah.size==tvdss.size==x.size==y.size:
            raise ValueError("dah, tvdss, x and y must be the same size")

        self._puttrack(self._handle, dah, tvdss, x, y, dah.size)
        if not self.isok:
            raise ValueError(self.errmsg)

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
 
