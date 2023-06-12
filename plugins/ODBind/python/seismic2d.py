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
        clss._compnames = wrap_function(LIBODB, f'{bindnm}_compnames', ct.c_void_p, [ct.c_void_p])
        clss._linenames = wrap_function(LIBODB, f'{bindnm}_linenames', ct.c_void_p, [ct.c_void_p])
        clss._lineinfo = wrap_function(LIBODB, f'{bindnm}_lineinfo', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._getdata = wrap_function(LIBODB, f'{bindnm}_getdata', None, [ct.c_void_p, NumpyAllocator.CFUNCTYPE, ct.c_char_p, ct.POINTER(ct.c_float)])

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
    def comp_names(self) ->list[str]:
        """list[str]: Names of components in this seismic dataset (readonly)"""
        return pystrlist(self._compnames(self._handle))

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

    def getdata(self, linenm: str) ->tuple:
        """Read the data for the given linenm

        Reads all components and various supporting data and returns a tuple of:
        -  a numpy array for each seismic component
        -  a Python dict with information about the data

        The information dict has the following keys and data:
        -  'comp': list[str] of the seismic component names
        -  'line': str of the line name
        -  'trc': np.ndarray(int) with the trace numbers
        -  'ref': np.ndarray(float) with the SP numbers
        -  'x': np.ndarray(double) with the x coordinates of the traces  
        -  'y': np.ndarray(double) with the y coordinates of the traces
        -  'twt' | 'depth': list[float] with the Z start, stop and step (in display units)
        -  'dims': list[str] dimensions of the trace data

        Parameters
        ----------
        linenm : str
            line name to read

        Returns
        -------
        tuple : list[np.ndarrays], one array per seismic component and info dict

        """
        allocator = NumpyAllocator()
        zrg = (ct.c_float * 3)()

        self._getdata(self._handle, allocator.cfunc, linenm.encode(), zrg)
        if not self.isok:
            raise ValueError(self.errmsg)

        zdim = 'twt' if self.zistime else 'depth'
        dims = ['trc', zdim]
        
        compnms = self.comp_names
        info =  {
                    'comp': compnms,
                    'line': linenm,
                    'trc': allocator.allocated_arrays[-4],
                    'ref': allocator.allocated_arrays[-3],
                    'x': allocator.allocated_arrays[-2],
                    'y': allocator.allocated_arrays[-1],
                    zdim: zrg[:],
                    'dims': dims
                }
        return ([allocator.allocated_arrays[compnms.index(compnm)] for compnm in compnms], info,)

    def as_xarray(self, data: list[np.ndarray], info: dict ):
        """Convert data returned by getdata method to Xarray Dataset

        Parameters
        ----------
        data : list[np.ndarray]
            seismic data for each of the components, in trc, z axis order
        info : dict
            as returned by the getdata method, at minimum require 'trc, 'xline' and 'twt|depth' fields

        Returns
        -------
        Xarray.Dataset

        """
        from xarray import DataArray, Dataset
        si = self._survey.info()
        di = self.info()
        zdim = 'twt' if self.zistime else 'depth'
        dims_xy = [dim for dim in info['dims'] if dim!=zdim]
        xyattrs = {'units': si['xyunit']}
        zattrs = {'units': di['zunit']}
        zrg = info[zdim]
        coords =    {
                        'line': info['line'],
                        'trc': info['trc'],
                        'cdp_x': DataArray(info['x'], dims=['trc'], attrs=xyattrs),
                        'cdp_y': DataArray(info['y'], dims=['trc'], attrs=xyattrs),
                        zdim: DataArray(np.arange(zrg[0],zrg[1]+zrg[2],zrg[2], dtype=np.float32), dims=[zdim], attrs=zattrs)
                    }
        attribs =   {
                        'description': di['name'],
                        'units': di['zunit'],
                        'crs': si['crs']
                    }
        return Dataset(data_vars={dv: (info['dims'], data[idx]) for idx, dv in enumerate(info['comp'])}, coords=coords, attrs=attribs)

Seismic2D._initbindings('seismic2d')

