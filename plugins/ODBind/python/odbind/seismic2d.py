import numpy as np
import ctypes as ct
from collections import namedtuple
from collections.abc import Sequence
from enum import Enum
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del, unpack_slice, is_none_slice 
from odbind.survey import Survey, _SurveyObject
from odbind.geom2d import Geom2D

class Seismic2D(_SurveyObject):
    """
    A class for an OpendTect 2D seismic dataset
    """

    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._newout = wrap_function(LIBODB, f'{bindnm}_newout', ct.c_void_p, [ct.c_void_p, ct.c_char_p, ct.c_char_p, ct.c_void_p,
                                                                                ct.c_bool, ct.c_bool])
        clss._close = wrap_function(LIBODB, f'{bindnm}_close', None, [ct.c_void_p])
        clss._compnames = wrap_function(LIBODB, f'{bindnm}_compnames', ct.c_void_p, [ct.c_void_p])
        clss._linenames = wrap_function(LIBODB, f'{bindnm}_linenames', ct.c_void_p, [ct.c_void_p])
        clss._lineinfo = wrap_function(LIBODB, f'{bindnm}_lineinfo', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_void_p])
        clss._getdata = wrap_function(LIBODB, f'{bindnm}_getdata', None, [ct.c_void_p, NumpyAllocator.CFUNCTYPE, ct.c_char_p, ct.POINTER(ct.c_float)])
        putdataargs = [ ct.c_void_p, ct.c_char_p, ct.POINTER(ct.POINTER(ct.c_float)), ct.c_int, ct.c_int,
                        ct.POINTER(ct.c_float), ct.POINTER(ct.c_int)
                    ]
        clss._putdata = wrap_function(LIBODB, f'{bindnm}_putdata', None, putdataargs)
        clss._dellines = wrap_function(LIBODB, f'{bindnm}_deletelines', ct.c_bool, [ct.c_void_p, ct.c_void_p])

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

    @classmethod
    def create(clss, survey: Survey, name: str, components: list[str]=['Comp1'], fmt: str='CBVS', 
                zistime: bool=True, overwrite: bool=False):
        """Create a new OpendTect 2D seismic dataset object

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        name : str
            OpendTect 2D seismic dataset name
        compnames : list[str] = ['Comp1']
            Output component names
        format : str='CBVS'
            Output format type string, either 'CBVS' or 'SEGYDirect'
        zistime : bool=True
            Flag to indicate the Z domain of the new dataset
        overwrite : bool=False
            Flag to indicate if the new dataset can replace an existing dataset of the same name

        Returns
        -------
        A Seismic2D object

        """

        newseis = clss.__new__(clss)
        newseis._handle = None
        newseis._survey = survey
        compnmsptr = makestrlist(components)
        newseis._handle = clss._newout(survey._handle, name.encode(), fmt.encode(), compnmsptr, zistime, overwrite)
        stringset_del(compnmsptr)
        if not newseis._handle or not survey.isok:
            raise NameError(survey.errmsg)
        elif not newseis.isok:
            raise NameError(newseis.errmsg)

        return newseis

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def close(self):
        self._close(self._handle)

    def line_info(self, forlinenms: list[str]=[]):
        """Return basic information for all or a subset of lines in this 2D dataset.
        
        Parameters
        ----------
        forlinenms : list[str]=[]
            (Optional) a list of line names to use. For an empty list information for all lines in the dataset is provided.
        
        Returns
        -------
        dict or Pandas DataFrame depending on the value of Seismic2D.use_dataframe

        """
        fornmsptr = makestrlist(forlinenms)
        infolist = pyjsonstr(self._lineinfo(self._handle, fornmsptr ))
        stringset_del(fornmsptr)
        return self.dictlist_to_dataframe(infolist) if Seismic2D.use_dataframe else infolist 

    def getdata(self, linenm: str):
        """Return the 2D seismic data for the given linenm

        Reads all components and various supporting data. Return format determined by
        current setting of Seismic2D.use_xarray.
        
        For Seismic2D.use_xarray == True:
            - The data is returned as an Xarray.Dataset with a data variable for each seismic 
            component. 
            - Each data variable will be a 2D Xarray Dataarray with trace number as the first 
            axis of trace number and z sample index as the second (last) axis.
            - The coordinates defined in the Xarray dataset are:
                - line: the line name
                - trc: the trace numbers
                - ref: the source/shot point number at the trace location
                - x: Trace easting in the survey CRS
                - y: Trace northing in the survey CRS
                - twt/depth: Trace z samples in milliseconds for twt and m/ft for depth
            - The OpendTect data object name, survey CRS and units are added to the Xarray Dataset 
            as attributes.

        For Seismic2D.use_xarray == False:
            - The data is returned as a tuple of:
                -  a list of numpy arrays, one for each seismic component
                -  a Python dict with information about the data
            - The information dict has the following keys and data:
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
        or
        Xarray.Dataset depending on the value of Seismic2D.use_xarray

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
        data = [allocator.allocated_arrays[compnms.index(compnm)] for compnm in compnms]
        return self.to_xarray(data, info) if Seismic2D.use_xarray else (data, info,)

    def to_xarray(self, data: list, info: dict):
        """Convert 2D seismic data in simple list+dict format to an Xarray Dataset

        See Seismic2D.getdata for details of the input and output formats.
        
        Parameters
        ----------
        data : list[np.ndarrays], one array per seismic component
        info : dict

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
        ns = 1 if isinstance(zrg, float) else round((zrg[1]-zrg[0])/zrg[2])+1
        coords =    {
                        'line': info['line'],
                        'trc': info['trc'],
                        'ref': DataArray(info['ref'], dims=['trc']),
                        'x': DataArray(info['x'], dims=['trc'], attrs=xyattrs),
                        'y': DataArray(info['y'], dims=['trc'], attrs=xyattrs),
                        zdim: DataArray(np.linspace(zrg[0], zrg[1], ns,endpoint=True, dtype=np.float32), dims=[zdim], attrs=zattrs)
                    }
        attribs =   {
                        'description': di['name'],
                        'units': di['zunit'],
                        'crs': si['crs']
                    }
        return Dataset(data_vars={dv: (info['dims'], data[idx]) for idx, dv in enumerate(info['comp'])}, coords=coords, attrs=attribs)

    def from_xarray(self, xrdata) ->tuple:
        """Convert 2D seismic data in Xarray.Dataset to tuple format

        See Seismic2D.getdata for details of the input and output formats.

        Parameters
        ----------
        xrdata : Xarray.Dataset

        Returns
        -------
        data : list[np.ndarrays], one array per seismic component
        info : dict

        """
        from xarray import DataArray, Dataset
        zdim = 'twt' if self.zistime else 'depth'
        data = [xrdata[key].to_numpy() for key in list(xrdata.data_vars)]
        info = { key: xrdata[key].to_numpy() for key in list(xrdata.coords) if key in ['trc','ref','x','y']}
        z0 = xrdata[zdim].to_numpy()[0]
        z1= xrdata[zdim].to_numpy()[-1]
        z2 = (z1-z0)/(data[0].shape[-1]-1)
        info[zdim] = [z0, z1, z2]
        info['line'] = xrdata['line']
        info['comp'] = list(xrdata.data_vars)
        info['dims'] = ['trc', zdim]
        return (data, info,)

    def putdata(self, linenm: str, indata, creategeom: bool, overwrite: bool):
        """Write a 2D seismic line from either a tuple or Xarray.Dataset of 2D seismic data

        See Seismic2D.getdata for details of the input formats.
        Note:
        - assumes components are in the same order as returned by comp_names property
        - if number of components>len(comp_names), extra data items are ignored
        - if number of components<len(comp_names), missing data items are zero filled
        - raises ValueError if geometry is incompatible with data shape
        - if creategeom=False only traces in the defined geometry are saved

        Parameters
        ----------
        linenm : str
            2D line name
        indata : tuple(list[np.ndarray], dict) | Xarray.Datatset
            see Seismic2D.getdata for details
        creategeom: bool
            if True and info contains the required information ('trc', 'ref', 'x', 'y' and 'twt|depth' fields)
            the 2D line geometry is created/overwritten
        overwrite: bool
            if True and the line name already exists in the dataset it is replaced

        """
        data = []
        info = {}
        if isinstance(indata, tuple):
            data = indata[0]
            info = indata[1]
        else:
            data, info = self.from_xarray(indata)

        compnms = self.comp_names
        zdim = 'twt' if self.zistime else 'depth'
        trcnrs = info['trc']
        ntrcs = len(trcnrs)
        if linenm in self.line_names:
            if overwrite:
                self.delete_lines([linenm])
            else:
                return
                
        if creategeom:
            allok = True
            for key in ['ref', 'x', 'y']:
                allok = allok and ntrcs==len(info[key])

            if allok:
                with Geom2D.create(self._survey, linenm, True) as geom:
                    geom.putdata(info)
            else:
                raise ValueError("inconsistent geometry information")

        zrg = info[zdim]
        zrg = zrg if isinstance(zrg, list) else [zrg, zrg, 1]
        datashp = []
        datashp.append(len(trcnrs))
        if zrg[0]!=zrg[1]:
            datashp.append(int((zrg[1]-zrg[0])/zrg[2])+1)


        datas = []
        for idx, compnm in enumerate(compnms):
            datum = np.zeros(datashp, dtype=np.float32)
            if idx<len(data):
                datum = data[idx]

            if datum.shape!=tuple(datashp):
                raise ValueError(f'ranges {tuple(datashp)} and data shape {datum.shape} are incompatible.')

            datum = np.ascontiguousarray(datum, dtype=np.float32)
            datas.append(datum.ctypes.data_as(ct.POINTER(ct.c_float)))

        dataptr = ((ct.POINTER(ct.c_float)) * len(data))(*datas)
        ct_zrg = (ct.c_float * 3)(*zrg)
        ct_trcnrs = (ct.c_int * len(trcnrs))(*trcnrs)
        self._putdata( self._handle, linenm.encode(), dataptr, datashp[0], datashp[1], ct_zrg, ct_trcnrs)
        if not self.isok:
            raise ValueError(self.errmsg)

    def delete_lines(self, linenms: list[str]=[]):
        """Delete the listed lines from the 2D seismic dataset

        Parameters
        ----------
        linenms : list[str]
            list of lines to be deleted

        Returns
        -------
        bool : True/False indicating success/failure

        """
        linenmsptr = makestrlist(linenms)
        res = self._dellines(self._handle, linenmsptr)
        stringset_del(linenmsptr)
        return res





Seismic2D._initbindings('seismic2d')

