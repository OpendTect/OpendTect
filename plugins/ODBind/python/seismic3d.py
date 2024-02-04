import numpy as np
import ctypes as ct
from collections import namedtuple
from collections.abc import Sequence
from enum import IntEnum
import xarray as xr
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del, unpack_slice, is_none_slice 
from odbind.survey import Survey, _SurveyObject

class SliceType(IntEnum):
    Inline = 0
    Crossline = 1
    ZSlice = 2

class Seismic3D(_SurveyObject):
    """
    A class for an OpendTect seismic volume
    """

    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._newout = wrap_function(LIBODB, f'{bindnm}_newout', ct.c_void_p, [ct.c_void_p, ct.c_char_p, ct.c_char_p, ct.c_void_p,
                                                                                ct.POINTER(ct.c_int), ct.POINTER(ct.c_int), 
                                                                                ct.POINTER(ct.c_float), ct.c_bool, ct.c_bool])
        clss._close = wrap_function(LIBODB, f'{bindnm}_close', None, [ct.c_void_p])
        clss._compnames = wrap_function(LIBODB, f'{bindnm}_compnames', ct.c_void_p, [ct.c_void_p])
        clss._nrbins = wrap_function(LIBODB, f'{bindnm}_nrbins', ct.c_int, [ct.c_void_p])
        clss._nrtrcs = wrap_function(LIBODB, f'{bindnm}_nrtrcs', ct.c_int, [ct.c_void_p])
        clss._getzidx = wrap_function(LIBODB, f'{bindnm}_getzidx', ct.c_int, [ct.c_void_p, ct.c_float])
        clss._getzval = wrap_function(LIBODB, f'{bindnm}_getzval', ct.c_float, [ct.c_void_p, ct.c_int])
        clss._gettrcidx = wrap_function(LIBODB, f'{bindnm}_gettrcidx', ct.c_int, [ct.c_void_p, ct.c_int, ct.c_int])
        clss._getinlcrl = wrap_function(LIBODB, f'{bindnm}_getinlcrl', None, [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_int), ct.POINTER(ct.c_int)])
        clss._zrange = wrap_function(LIBODB, f'{bindnm}_zrange', None, [ct.c_void_p, ct.POINTER(ct.c_float)])
        clss._getdata = wrap_function(LIBODB, f'{bindnm}_getdata', None, [ct.c_void_p, NumpyAllocator.CFUNCTYPE,
                                                                            ct.POINTER(ct.c_int), ct.POINTER(ct.c_int), ct.POINTER(ct.c_float)                    
                                                                        ])
        putdataargs = [ ct.c_void_p, ct.POINTER(ct.POINTER(ct.c_float)),
                        ct.POINTER(ct.c_int), ct.POINTER(ct.c_int), ct.POINTER(ct.c_float)
                    ]
        clss._putdata = wrap_function(LIBODB, f'{bindnm}_putdata', None, putdataargs)

    def __init__(self, survey: Survey, name: str):
        """Initialise an OpendTect 3D seismic volume object

        Parameters
        ----------
        survey : Survey
            an OpendTect survey object
        name : str
            an OpendTect 3D seismic dataset name

        """
        super(Seismic3D, self).__init__(survey, name)
        self._trace = Trace3D(self)
        self._volume = Volume3D(self)
        self._iline = Slice3D(self, SliceType.Inline)
        self._xline = Slice3D(self, SliceType.Crossline)
        self._zslice = Slice3D(self, SliceType.ZSlice)

    @property
    def bin_count(self) ->int:
        """int : Number of bins within the extent of this seismic volume, number_of_bins>=number_of_traces"""
        return self._nrbins(self._handle)

    @property
    def comp_names(self) ->list[str]:
        """list[str]: Names of components in this seismic volume (readonly)"""
        return pystrlist(self._compnames(self._handle))

    @property
    def trace(self):
        """
        Return object for trace mode access to the OpendTect seismc volume.

        Returns
        -------
        Trace3D : an object providing [trace_number], [inline, crossline], [trace_number slice] or [inline slice, crossline slice] indexing
            into an OpendTect 3D seismic volume.

        """
        return self._trace

    @trace.setter
    def trace(self, val):
        self.trace[:,:] = val 

    @property
    def volume(self):
        """
        Return object for volume mode access to the OpendTect seismc volume.
        
        Returns
        -------
        Volume3D : an object providing [inline slice, crossline slice, zsample slice] indexing
            into an OpendTect 3D seismic volume.

        """
        return self._volume

    @volume.setter
    def volume(self, val):
        self.volume[:] = val 

    @property
    def iline(self):
        """
        Return object for inline mode access to the OpendTect seismc volume.
        
        Returns
        -------
        Slice3D : an object providing [inline_number] and [inline_slice] indexing
            into an OpendTect 3D seismic volume.

        """
        return self._iline

    @iline.setter
    def iline(self, val):
        self.iline[:] = val 

    @property
    def xline(self):
        """
        Return object for crossline mode access to the OpendTect seismc volume.
        
        Returns
        -------
        Slice3D : object providing [crossline_number] and [crossline_slice] indexing
            into an OpendTect 3D seismic volume.

        """
        return self._xline

    @xline.setter
    def xline(self, val):
        self.xline[:] = val 

    @property
    def zslice(self):
        """
        Return object for Z slice mode access to the OpendTect seismc volume.
        
        Returns
        -------
        Slice3D : an object providing [zslice_number] and [zslice_slice] indexing
            into an OpendTect 3D seismic volume.

        """
        return self._zslice

    @property
    def trace_count(self) ->int:
        """int : Expected number of traces in this seismic volume."""
        res = self._nrtrcs(self._handle)
        if not self.isok:
            raise IndexError(self.errmsg)

        return res

    @property
    def ranges(self) ->namedtuple:
        """namedtuple[inlrg, crlrg, zrg]: inline, crossline and z range of 3D seismic volume (readonly)"""
        Sampling = namedtuple('Sampling', ['inlrg', 'crlrg', 'zrg'])
        info = self.info()
        return Sampling(info['inl_range'], info['crl_range'], info['z_range'])

    @classmethod
    def create(clss, survey: Survey, name: str, inl_rg: list[int], crl_rg: list[int], z_rg: list[float], components: list[str]=['Component 1'], 
                fmt: str='CBVS', zistime: bool=True, overwrite: bool=False):
        """Create a new OpendTect 3D seismic volume object

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        name : str
            OpendTect 3D seismic volume name
        inl_rg : list[int]
        crl_rg : list[int]
        z_rg : list[float]
        compnames : list[str] = ['Component 1']
            Output component names
        format : str='CBVS'
            Output format type string, either 'CBVS' or 'SEGYDirect'
        zistime : bool=True
            Flag to indicate the Z domain of the new volume
        overwrite : bool=False
            Flag to indicate if the new volume can replace an existing volume of the same name

        Returns
        -------
        A Seismic3D object

        """

        newseis = clss.__new__(clss)
        newseis._handle = None
        newseis._survey = survey
        ct_inlrg = (ct.c_int * 3)(*inl_rg)
        ct_crlrg = (ct.c_int * 3)(*crl_rg)
        ct_zrg = (ct.c_float * 3)(*z_rg)
        compnmsptr = makestrlist(components)
        newseis._handle = clss._newout(survey._handle, name.encode(), fmt.encode(), compnmsptr, ct_inlrg, ct_crlrg, ct_zrg, zistime, overwrite)
        stringset_del(compnmsptr)
        if not newseis._handle or not survey.isok:
            raise TypeError(survey.errmsg)
        elif not newseis.isok:
            raise TypeError(newseis.errmsg)

        newseis._trace = Trace3D(newseis)
        newseis._volume = Volume3D(newseis)
        newseis._iline = Slice3D(newseis, SliceType.Inline)
        newseis._xline = Slice3D(newseis, SliceType.Crossline)
        newseis._zslice = Slice3D(newseis, SliceType.ZSlice)
        return newseis

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self._close(self._handle)

    def close(self):
        self._close(self._handle)

    def z_index(self, zval: float) ->int:
        """Return the z index for a given z value in this seismic volume

        A returned index of -1 corresponds to a zval outside the dataset

        Parameters
        ----------
        zval : float
            the z value.

        Returns
        -------
        int : the corresponding z index or -1 if the z value is outside the seismic volume

        """
        res =self._getzidx(self._handle, zval)
        if not self.isok:
            raise IndexError(self.errmsg)

        return res

    def z_value(self, zidx: int) ->float:
        """Return the z value for a given z index in this seismic volume

        if zidx is outside the datset returns NaN

        Parameters
        ----------
        zidx : int
            the z index.

        Returns
        -------
        float : the corresponding z value or NaN if the z index is outside the seismic volume

        """
        res =self._getzval(self._handle, zidx)
        if not self.isok:
            raise IndexError(self.errmsg)

        return res



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
        res =self._gettrcidx(self._handle, inline, crossline)
        if not self.isok:
            raise IndexError(self.errmsg)

        return res

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
        inline = ct.c_int()
        crline = ct.c_int()
        self._getinlcrl(self._handle, trace_index, ct.byref(inline), ct.byref(crline))
        if not self.isok:
            raise IndexError(self.errmsg)

        return (inline.value, crline.value)

    def getdata(self, inlrg: list[int], crlrg: list[int], zrg: list[float]):
        """Read a rectangular block of data from the 3D seismic volume

        Reads all components and various supporting data. Return format determined by
        current setting of Seismic3D.use_xarray.
        
        For Seismic3D.use_xarray == True:
            - The data is returned as an Xarray.Dataset with a data variable for each seismic 
            component.
            - The axes of the data variables will depend on the dimensionality of the data:
                - Single Trace: 1D Xarray Dataarray, with z sample index as the single axis.
                - Inline/Crossline Slice: 2D Xarray Dataarray, with first axis of crossline 
                number for Inline Slices and inline number for Crossline Slices. The second 
                (last) axis is the z sample index. 
                - Z Slice: 2D Xarray Dataarray, with inline number as the first axis and 
                crossline number as the second axis.
                - Subvolume: 3D Xarray Dataarray, with inline number as the first axis, 
                crossline number as the second axis and z sample index as the third (last) axis.
            - The coordinates defined in the Xarray dataset are:
                - iline: inline number
                - xline: crossline number
                - x: Trace easting in the survey CRS
                - y: Trace northing in the survey CRS
                - twt/depth: Trace z samples in milliseconds for twt and m/ft for depth
            - The OpendTect data object name, survey CRS and units are added to the Xarray Dataset 
            as attributes.

        For Seismic3D.use_xarray == False:
            - The data is returned as a tuple of:
                -  a list of numpy arrays, one for each seismic component
                -  a Python dict with information about the data
            - The information dict has the following keys and data:
                -  'comp': list[str] of the seismic component names
                -  'iline': int | list[int] with the inline start, stop and step
                -  'xline': int | list[int] with the crossline start, stop and step
                -  'x': double | np.ndarray(double) with the x coordinates of the traces  
                -  'y': double | np.ndarray(double) with the y coordinates of the traces
                -  'twt' | 'depth': float | list[float] with the Z start, stop and step (in display units)
                -  'dims': list[str] dimensions of the trace data

        Parameters
        ----------
        inlrg : list[int]
            Inline start, stop and step to read
        crlrg : list[int]
            Crossline start, stop and step to read
        zrg : list[float]
            Z start, stop and step to read (in display units)

        Returns
        -------
        tuple(list[np.ndarray], dict) | Xarray.Datatset
            see above for details

        """
        allocator = NumpyAllocator()
        ct_inlrg = (ct.c_int * 3)(*inlrg)
        ct_crlrg = (ct.c_int * 3)(*crlrg)
        ct_zrg = (ct.c_float * 3)(*zrg)
        self._getdata(self._handle, allocator.cfunc, ct_inlrg, ct_crlrg, ct_zrg)
        if not self.isok:
            raise ValueError(self.errmsg)

        dims = []
        if inlrg[0]!=inlrg[1]:
            dims.append('iline')
        
        if crlrg[0]!=crlrg[1]:
            dims.append('xline')

        zdim = 'twt' if self.zistime else 'depth'
        if zrg[0]!=zrg[1]:
            dims.append(zdim)
        
        compnms = self.comp_names
        info =  {
                    'comp': compnms,
                    'iline': inlrg[0] if inlrg[0]==inlrg[1] else inlrg,
                    'xline': crlrg[0] if crlrg[0]==crlrg[1] else crlrg,
                    'x': allocator.allocated_arrays[-2],
                    'y': allocator.allocated_arrays[-1],
                    zdim: zrg[0] if zrg[0]==zrg[1] else zrg,
                    'dims': dims
                }
        data = [allocator.allocated_arrays[compnms.index(compnm)] for compnm in compnms]
        return self.to_xarray(data, info) if Seismic3D.use_xarray else (data, info,)

    def putdata(self, indata):
        """Write a 3D seismic volume from either a tuple or Xarray.Dataset of 3D seismic data

        See Seismic3D.getdata for details of the input formats.
        Note:
        - assumes components in 'data' list are in same order as returned by comp_names property
        - if number of components>len(comp_names), extra data items are ignored
        - if number of components<len(comp_names), missing data items are zero filled
        - raises ValueError if geometry in info dict is incompatible with data shape
        - only data compatible with the ranges specified in the seismic volume creation call 
        (ie inside and on the sample grid) are saved

        Parameters
        ----------
        indata : tuple(list[np.ndarray], dict) | Xarray.Datatset
            see Seismic3D.getdata for details
        info : dict
            at minimum require 'iline, 'xline' and 'twt|depth' fields

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
        inlrg = info['iline']
        inlrg = inlrg if isinstance(inlrg, list) else [inlrg, inlrg, 1]
        crlrg = info['xline']
        crlrg = crlrg if isinstance(crlrg, list) else [crlrg, crlrg, 1]
        zrg = info[zdim]
        zrg = zrg if isinstance(zrg, list) else [zrg, zrg, 1]
        datashp = []
        if inlrg[0]!=inlrg[1]:
            datashp.append(((inlrg[1]-inlrg[0])//inlrg[2])+1)

        if crlrg[0]!=crlrg[1]:
            datashp.append(((crlrg[1]-crlrg[0])//crlrg[2])+1)

        if zrg[0]!=zrg[1]:
            datashp.append(round((zrg[1]-zrg[0])/zrg[2])+1)
        datas = []
        for idx, compnm in enumerate(compnms):
            datum = np.zeros(datashp, dtype=np.float32)
            if idx<len(data):
                datum = data[idx]

            if datum.shape!=tuple(datashp):
                raise ValueError(f'ranges {tuple(datashp)} and data shape {datum.shape} are incompatible.')

            datum = np.ascontiguousarray(datum, dtype=np.float32)
            datas.append(datum.ctypes.data_as(ct.POINTER(ct.c_float)))

        dataptr = ((ct.POINTER(ct.c_float)) * len(compnms))(*datas)
        ct_inlrg = (ct.c_int * 3)(*inlrg)
        ct_crlrg = (ct.c_int * 3)(*crlrg)
        ct_zrg = (ct.c_float * 3)(*zrg)

        self._putdata( self._handle, dataptr, ct_inlrg, ct_crlrg, ct_zrg)
        if not self.isok:
            raise ValueError(self.errmsg)

    def putdata_byrange(self, data, inlrg: list[int], crlrg: list[int], zrg: list[float]):
        """Write a rectangular block of data  to a 3D seismic volume

        See notes of _putdata method

        Parameters
        ----------
        data : list|tuple[np.ndarray(float)]
        inlrg : list[int]
            Inline start, stop and step of the data
        crlrg : list[int]
            Crossline start, stop and step of the data
        zrg : list[float]
            Z start, stop and step of the data (in display units)


        """
        zdim = 'twt' if self.zistime else 'depth'
        info = {
                    'iline': inlrg,
                    'xline': crlrg,
                    zdim: zrg
                }
        self.putdata((data, info,))

    def to_xarray(self, data: list[np.ndarray], info: dict ):
        """Convert 3D seismic data in simple list+dict format to an Xarray Dataset

        See Seismic3D.getdata for details of the input and output formats.

        Parameters
        ----------
        data : list[np.ndarray]
            seismic data for each of the components, in iline, xline, z axis order
        info : dict
            at minimum require 'iline, 'xline' and 'twt|depth' fields

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
        inlrg = info['iline']
        crlrg = info['xline']
        zrg = info[zdim]
        ns = 1 if isinstance(zrg, float) else round((zrg[1]-zrg[0])/zrg[2])+1
        coords =    {
                        'iline': inlrg if isinstance(inlrg, int) else np.arange(inlrg[0],inlrg[1]+inlrg[2],inlrg[2], dtype=np.int32),
                        'xline': crlrg if isinstance(crlrg, int) else np.arange(crlrg[0],crlrg[1]+crlrg[2],crlrg[2], dtype=np.int32),
                        'x': info['x'][0] if len(info['x'])==1 else DataArray(info['x'], dims=dims_xy, attrs=xyattrs),
                        'y': info['y'][0] if len(info['y'])==1 else DataArray(info['y'], dims=dims_xy, attrs=xyattrs),
                        zdim: zrg if isinstance(zrg, float) else DataArray(np.linspace(zrg[0],zrg[1],ns,endpoint=True,dtype=np.float32), dims=[zdim], attrs=zattrs)
                    }
        attribs =   {
                        'description': di['name'],
                        'units': di['zunit'],
                        'crs': si['crs']
                    }
        return Dataset(data_vars={dv: (info['dims'], data[idx]) for idx, dv in enumerate(info['comp'])}, coords=coords, attrs=attribs)

    def from_xarray(self, xrdata) ->tuple:
        """Convert 3D seismic data in Xarray.Dataset to simple list+dict format

        See Seismic3D.getdata for details of the input and output formats.
        Note:
        - only coordinates iline, xline and twt|depth need to be present in the dataset

        Parameters
        ----------
        xrdata : Xarray.Dataset

        Returns
        -------
        data : list[np.ndarrays], one array per seismic component
        info : dict

        """
        zdim = 'twt' if self.zistime else 'depth'
        data = [xrdata[key].to_numpy() for key in list(xrdata.data_vars)]
        info = { key: xrdata[key].to_numpy() for key in list(xrdata.coords) if key in ['iline','xline']}
        il0 = xrdata['iline'].item() if xrdata['iline'].size==1 else xrdata['iline'][0].item()
        il1 = xrdata['iline'].item() if xrdata['iline'].size==1 else xrdata['iline'][-1].item()
        il2 = 1 if xrdata['iline'].size==1 else round((il1-il0)/(data[0].shape[0]-1))
        xl0 =  xrdata['xline'].item() if xrdata['xline'].size==1 else xrdata['xline'][0].item()
        xl1 = xrdata['xline'].item() if xrdata['xline'].size==1 else xrdata['xline'][-1].item()
        xl2 = 1 if xrdata['xline'].size==1 else round((xl1-xl0)/(data[0].shape[-2]-1))
        z0 = xrdata[zdim].item() if xrdata[zdim].size==1 else xrdata[zdim][0].item()
        z1= xrdata[zdim].item() if xrdata[zdim].size==1 else xrdata[zdim][-1].item()
        z2 = 1 if xrdata[zdim].size==1 else(z1-z0)/(data[0].shape[-1]-1)
        info['iline'] = [il0, il1, il2]
        info['xline'] = [xl0, xl1, xl2]
        info[zdim] = [z0, z1, z2]
        info['comp'] = list(xrdata.data_vars)
        dims = []
        if il0!=il1:
            dims.append('iline')
        
        if xl0!=xl1:
            dims.append('xline')

        if z0!=z1:
            dims.append(zdim)

        info['dims'] = dims
        return (data, info,)

    def is_seisdata(self, data) ->bool:
        from xarray import Dataset
        zdim = 'twt' if self.zistime else 'depth'
        if isinstance(data, (list, tuple,)):
            return len(data)==2 \
                and isinstance(data[0], list) \
                and isinstance(data[1], dict) \
                and 'iline' in data[1] \
                and 'xline' in data[1] \
                and zdim in data[1] \
                and all(isinstance(itm, np.ndarray) for itm in data[0])
        elif isinstance(data, Dataset):
            return True
        else:
            return False

Seismic3D._initbindings('seismic3d')

class Trace3D(Sequence):
    def __init__(self, seis: Seismic3D ):
        self._seis3d = seis

    @property
    def seis3d(self) ->Seismic3D:
        return self._seis3d

    def wrapindex(self, i):
        if i < 0:
            i += len(self)

        if not 0 <= i < len(self):
            raise IndexError('trace index out of range')

        return i

    def __getitem__(self, idx):
        """Either trace[trace_number], trace[inl, crl], trace[trace_number_slice] or trace[inline_slice, crossline_slice]

        [trace_number] returns a single trace, negative indices wrap around.
        [inl, crl] return the single trace at the given iln, crl location. Negatice indices don't wrap. 
        Raises an IndexError exception if the inl, crl location is outside the volume .
        [slice] return a generator for a range of trace numbers.
        [slice, slice] return a generator for all traces in a rectangular subvolume where the 2 slices 
        specify the inline and crossline ranges.

        Note:
        - slice ranges are inclusive of both ends (unlike Python ranges which normally exclude the end index).
        - for trace number indices and slices negative indices wrap around.
        - negative inline/crossline indices and slices are allowed but don't cause wrap around. 
        - OpendTect 3D seismic data is generally stored for fast access along inlines (ie crossline number 
        changes more rapidly). Incremental access to traces by trace number will therefore access the data 
        so that the crossline number changes most rapidly.    
        
        Examples:
            [200] return the 200th trace in the volume
            [-1] return the last trace in the volume
            [300, 400] return the trace at inline 300 and crossline 400
            [200:200, :] returns a generator for all traces on inline 200
            [:, 300:300] returns a generator for all traces on crossline 300
            [200:300,400:450] returns a generator for all traces in a subvolume of 101 inlines, 51 crosslines.
        
        Parameters
        ----------
        idx : int | (int, int) | slice | (slice, slice)
        
        Returns
        -------
        tuple(list[np.ndarray], dict) | Xarray.Datatset | generator
            see Seismic3D.getdata for output format details

        """
        sampling = self.seis3d.ranges
        if isinstance(idx, int):
            idx = self.wrapindex(idx)
            inl, crl = self.seis3d.bin(idx)
            return self.seis3d.getdata([inl,inl,1], [crl,crl,1], sampling.zrg)
        elif isinstance(idx, tuple) and len(idx)==2 and isinstance(idx[0], int) and isinstance(idx[1], int):
            inl, crl = idx
            return self.seis3d.getdata([inl,inl,1], [crl,crl,1], sampling.zrg)
        elif isinstance(idx, tuple) and len(idx)==2 and isinstance(idx[0], slice) and isinstance(idx[1], slice):
            inls, crls = idx
            inlrg = unpack_slice(inls, sampling.inlrg)
            crlrg = unpack_slice(crls, sampling.crlrg)
            def gen():
                for inl in range(inlrg[0], inlrg[1]+inlrg[2],inlrg[2]):
                    for crl in range(crlrg[0], crlrg[1]+crlrg[2],crlrg[2]):
                        data = self.seis3d.getdata([inl,inl,1], [crl,crl,1], sampling.zrg)
                        yield data

            return gen()
        elif isinstance(idx, slice):
            trcs = idx
            trcrg = unpack_slice(trcs, [0, len(self)-1, 1])
            def gen():
                for trc in range(trcrg[0], trcrg[1]+trcrg[2],trcrg[2]):
                    inl, crl = self.seis3d.bin(trc)
                    data = self.seis3d.getdata([inl,inl,1], [crl,crl,1], sampling.zrg)
                    yield data

            return gen()
        else:
            raise TypeError('index should be either [trc_number], [inl, crl], [trc_number_slice] or [inline_slice, crossline_slice].')

    def __setitem__(self, idx, data):
        """Write trace data to the associated OpendTect 3D seismic volume. Only supports index mode trace[:].

        Parameters
        ----------
        idx : [:]
        data: tuple(list[np.ndarray], dict) | Xarray.Datatset
            see Seismic3D.getdata for details
            
        """
        if self.seis3d.is_seisdata(data):
            if isinstance(idx, slice) and is_none_slice(idx):
                self.seis3d.putdata(data)
            else:
                raise TypeError('index should be [:].')
        else:
            try:
                for datum in data:
                    self[:] = datum
            except:
                raise TypeError(f'expected input of tuple[list[np.ndarray], dict] or Xarray.Dataset but got {data}')

    def __len__(self):
        """x.__len__() <==> len(x)"""
        return self.seis3d.trace_count

class Slice3D(Sequence):
    def __init__(self, seis: Seismic3D, slice_type: SliceType ):
        self._seis3d = seis
        self._slicetype = slice_type

    @property
    def seis3d(self) ->Seismic3D:
        return self._seis3d

    def __len__(self):
        """x.__len__() <==> len(x)"""
        sampling = self.seis3d.ranges
        sdx = self._slicetype
        if sdx==SliceType.ZSlice:
            return int((sampling.zrg[1]-sampling.zrg[0])/sampling.zrg[2]) + 1
        else:
            return (sampling[sdx][1]-sampling[sdx][0])//sampling[sdx][2] + 1

    def slicerg_for(self, idx: int) ->tuple[list]:
        sampling = self.seis3d.ranges
        sdx = self._slicetype
        inlrg = sampling.inlrg
        crlrg = sampling.crlrg
        zrg = sampling.zrg
        if sdx==SliceType.ZSlice:
            zval = self.seis3d.z_value(idx)
            zrg = [zval, zval, zrg[2]]
        elif sdx==SliceType.Inline:
            inlrg = [idx, idx, 1]
        else:
            crlrg = [idx, idx, 1]

        return inlrg, crlrg, zrg



    def __getitem__(self, idx):
        """Either [number] or [number_slice] for slice types of inline, crossline or zsclice.

        [number] returns a single inline, crossline or zslice depending on the slice type.
        [number_slice] return a generator for a range of inline, crossline or zslice numbers depending on the slice type.
        For Z slices the index is the sample index. 

        Note:
        - slice ranges are inclusive of both ends (unlike Python ranges which normally exclude the end index).
        - negative inline/crossline indices and slices are allowed but don't cause wrap around. 

        Examples:
            [200] return inline, crossline or zslice 200 from the seismic volume.
            [400:500] returns a generator for inlines, crosslines or zslices 400 to 500
            [400:500:5] returns a generator for inlines 400, 405, 410,..., 495, 500
        
        Parameters
        ----------
        idx : int | slice
        
        Returns
        -------
        tuple(list[np.ndarray], dict) | Xarray.Datatset | generator
            see Seismic3D.getdata for output format details

        """
        if isinstance(idx, int):
            inlrg, crlrg, zrg = self.slicerg_for(idx)
            return self.seis3d.getdata(inlrg, crlrg, zrg)
        elif isinstance(idx, slice):
            sampling = self.seis3d.ranges
            sdx = self._slicetype
            mainrg = []
            if sdx==SliceType.ZSlice:
                zrg = sampling.zrg
                mainrg = [self.seis3d.z_index(zrg[0]), self.seis3d.z_index(zrg[1]), 1]
            elif sdx==SliceType.Inline:
                mainrg = sampling.inlrg
            else:
                mainrg = sampling.crlrg

            slrg = unpack_slice(idx, mainrg)
            def gen():
                for sdx in range(slrg[0], slrg[1]+slrg[2],slrg[2]):
                    inlrg, crlrg, zrg = self.slicerg_for(sdx)
                    data = self.seis3d.getdata(inlrg, crlrg, zrg)
                    yield data

            return gen()
        else:
            raise TypeError('index should be [number] or [slice].')

    def __setitem__(self, idx, data):
        """Write a slice to the associated OpendTect 3D seismic volume. Only supports index mode iline|xline|zslice[:].

        Parameters
        ----------
        idx : [:]
        data : tuple(list[np.ndarray], dict) | Xarray.Datatset
              see Seismic3D.getdata for output format details


        """
        if self._slicetype!=SliceType.Inline:
            raise TypeError(f'This operation only supports inline slices.')

        if self.seis3d.is_seisdata(data):
            if isinstance(idx, slice) and is_none_slice(idx):
                self.seis3d.putdata(data)
            else:
                raise TypeError('index should be [:].')
        else:
            try:
                for datum in data:
                    self[:] = datum
            except:
                raise TypeError(f'expected input of tuple[list[np.ndarray], dict] or Xarray.Dataset but got {data}')

class Volume3D():
    def __init__(self, seis: Seismic3D ):
        self._seis3d = seis

    @property
    def seis3d(self) ->Seismic3D:
        return self._seis3d

    def __getitem__(self, idx):
        """Get a rectangular subvolume using either volume(inline_slice, crossline_slice, Z_range_list) or volume(inline_slice, crossline_slice, zsample_slice)

        [slice, slice, slice] return a volume based on inline, crossline and z sample index
        [slice, slice, [start, stop, step]] return a volume based on inline, crossline and z range
        
        Note: 
        - the slice ranges are inclusive of both ends (unlike Python ranges which normally exclude the end index).
        - The returned data will have axes ordered inline, crossline, z.

        Examples:
        -   [200:300,400:450,[200,800,4]] returns a subvolume of 101 inlines, 51 crosslines and 151 z samples 
        between 200 and 800 ms/m depending on the Z domain.
        -   [200:300,400:450,50:200:1] returns a subvolume of 101 inlines, 51 crosslines and 151 z samples. 
        The Z range will depend on the data z sampling. Assuming it is a twt volume, z sampling starts at 0 
        and the sampling step is 4ms, the Z range of the subvolume would be 200 to 800ms.
        
        Parameters
        ----------
        idx : (slice,slice,list) or (slice, slice, slice)
        
        Returns
        -------
        tuple(list[np.ndarray], dict) | Xarray.Datatset
            see Seismic3D.getdata for output format details

        """
        sampling = self.seis3d.ranges
        zrg = sampling.zrg
        zsamprg = [self.seis3d.z_index(zrg[0]), self.seis3d.z_index(zrg[1]), 1]
        inls = None
        crls = None
        zrg = []
        if isinstance(idx, tuple) and len(idx)==3 and isinstance(idx[2], list) and isinstance(idx[1], slice) and isinstance(idx[0], slice):
            inls, crls, zrg = idx
        elif isinstance(idx, tuple) and len(idx)==3 and isinstance(idx[2], slice) and isinstance(idx[1], slice) and isinstance(idx[0], slice):
            inls, crls, zs = idx
            zs = unpack_slice(zs,zsamprg)
            zrg = [self.seis3d.z_value(zs[0]), self.seis3d.z_value(zs[1]), zs[2]*sampling.zrg[2]]
        else:
            raise TypeError('index should be [inline_slice, crossline_slice, Z_range_list] or [inline_slice, crossline_slice, zsample_slice].')

        inlrg = unpack_slice(inls, sampling.inlrg)
        crlrg = unpack_slice(crls, sampling.crlrg)
        return self.seis3d.getdata(inlrg, crlrg, zrg)

    def __setitem__(self, idx, vol):
        """Write a rectangular subvolume to the associated OpendTect 3D seismic volume. Only supports index mode volume[:].

        Parameters
        ----------
        idx : [:]
        vol : tuple(list[np.ndarray], dict) | Xarray.Datatset
              see Seismic3D.getdata for output format details

        """
        if self.seis3d.is_seisdata(vol):
            if isinstance(idx, slice) and is_none_slice(idx):
                self.seis3d.putdata(vol)
            else:
                raise TypeError('index should be [:].')
        else:
            raise TypeError(f'expected input of tuple[list[np.ndarray], dict] or Xarray.Dataset but got {vol}')


