import numpy as np
import ctypes as ct
from collections import namedtuple
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del 
from odbind.survey import Survey, _SurveyObject

class Horizon3D(_SurveyObject):
    """
    A class for an OpendTect 3D horizon
    """
    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._newout = wrap_function(LIBODB, f'{bindnm}_newout', ct.c_void_p, [ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_int), ct.POINTER(ct.c_int), ct.c_bool])
        clss._attribnames = wrap_function(LIBODB, f'{bindnm}_attribnames', ct.c_void_p, [ct.c_void_p])
        clss._getz = wrap_function(LIBODB, f'{bindnm}_getz', ct.c_void_p, [ct.c_void_p, NumpyAllocator.CFUNCTYPE])
        clss._getxy = wrap_function(LIBODB, f'{bindnm}_getxy', ct.c_void_p, [ct.c_void_p, NumpyAllocator.CFUNCTYPE])
        clss._getauxdata = wrap_function(LIBODB, f'{bindnm}_getauxdata', ct.c_void_p, [ct.c_void_p, NumpyAllocator.CFUNCTYPE, ct.c_char_p])
        putzargs = [ct.c_void_p, 
                    np.ctypeslib.ndpointer(dtype=np.float32, ndim=2, flags="C_CONTIGUOUS"),
                    ct.POINTER(ct.c_int), ct.POINTER(ct.c_int)
                    ]
        clss._putz = wrap_function(LIBODB, f'{bindnm}_putz', None, putzargs)
        putauxdataargs = [ ct.c_void_p, ct.c_char_p, np.ctypeslib.ndpointer(dtype=np.float32, ndim=2, flags="C_CONTIGUOUS"),
                           ct.POINTER(ct.c_int), ct.POINTER(ct.c_int)
                         ]
        clss._putauxdata = wrap_function(LIBODB, f'{bindnm}_putauxdata', None, putauxdataargs)
        clss._delattribs = wrap_function(LIBODB, f'{bindnm}_deleteattribs', ct.c_bool, [ct.c_void_p, ct.c_void_p])


    @property
    def attribnames(self) ->list[str]:
        """list[str]: Names of attributes attached to this 3D horizon (readonly)"""
        return pystrlist(self._attribnames(self._handle))

    @property
    def ranges(self) ->namedtuple:
        """namedtuple[inlrg, crlrg, zrg]: inline and crossline of the 3D horizon (readonly)"""
        Sampling = namedtuple('Sampling', ['inlrg', 'crlrg'])
        info = self.info()
        return Sampling(info['inl_range'], info['crl_range'])

    @classmethod
    def create(clss, survey: Survey, name: str, inl_rg: list[int], crl_rg: list[int], overwrite: bool=False):
        """Create a new OpendTect 3D horizon object

        Parameters
        ----------
        survey : Survey
            An OpendTect survey object
        name : str
            OpendTect 3D horizon name
        inl_rg : list[int]
            The inline range (start, stop and step) for the horizon
        crl_rg : list[int]
            The crossline range (start, stop and step) for the horizon
        overwrite : bool=False
            Flag to indicate if the new horizon can replace an existing horizon of the same name

        Returns
        -------
        A Horizon3D object

        """

        newhor = clss.__new__(clss)
        newhor._survey = survey
        ct_inlrg = (ct.c_int * 3)(*inl_rg)
        ct_crlrg = (ct.c_int * 3)(*crl_rg)
        newhor._handle = clss._newout(survey._handle, name.encode(), ct_inlrg, ct_crlrg, overwrite)
        if not newhor._handle or not survey.isok:
            raise TypeError(survey.errmsg)
        elif not newhor.isok:
            raise TypeError(newhor.errmsg)

        return newhor
    
    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        pass

    def get_z(self):
        """Get the 3D horizon Z values as a Numpy array

        Returns
        -------
        Numpy 2D array with the horizon Z values

        """

        allocator = NumpyAllocator()
        self._getz(self._handle, allocator.cfunc)
        if not self.isok:
            raise ValueError(self.errmsg)

        return allocator.allocated_arrays[0]
     
    def get_xy(self):
        """Get the 3D horizon X,Y values as Numpy arrays

        Returns
        -------
        Tuple of Numpy 2D arrays with the horizon X, Y values

        """

        allocator = NumpyAllocator()
        self._getxy(self._handle, allocator.cfunc)
        if not self.isok:
            raise ValueError(self.errmsg)

        return (allocator.allocated_arrays[:2])

    def get_attrib(self, attribname: str):
        """Get a 3D horizon attribute as a Numpy array

        Parameters
        ----------
        attribname : str
            The attribute to get

        Returns
        -------
        Numpy 2D array with the horizon attribute values

        """

        if attribname not in self.attribnames:
            raise ValueError("invalid attribute name")

        allocator = NumpyAllocator()
        self._getauxdata(self._handle, allocator.cfunc, attribname.encode())
        if not self.isok:
            raise ValueError(self.errmsg)

        return allocator.allocated_arrays[0]

    def getdata(self, attribnms : list[str]=[]):
        """Return the 3D horizon data

        Return format determined by current setting of Horizon3D.use_xarray.
        
        For Horizon3D.use_xarray == True:
            - The data is returned as a 2D Xarray.Dataset with the z values and horizon data as data variables.
            - The axes of the data variables are inline number (first) and crossline (second).
            - The coordinates defined in the Xarray dataset are:
                - iline: inline number
                - xline: crossline number
                - x: Trace easting in the survey CRS
                - y: Trace northing in the survey CRS
            - The OpendTect data object name, survey CRS and z value units are included as attributes.

        For Horizon3D.use_xarray == False:
            - The data is returned as a tuple of:
                -  a list of numpy arrays (length of the list must be the same as the length of the 'comp' info dict item)
                -  a Python dict with information about the data
            - The information dict has the following keys and data:
                -  'comp': list[str] names of data components as they are present in the data array, at minumum "z"
                -  'iline': list[int] with the inline start, stop and step
                -  'xline': list[int] with the crossline start, stop and step
                -  'x': double | np.ndarray(double) with the x coordinates of the traces  
                -  'y': double | np.ndarray(double) with the y coordinates of the traces

        Parameters
        ----------
        attribs : list[str] = []
            list of attributes to include, default is none

        Returns
        -------
         tuple(list[np.ndarray], dict) | Xarray.Datatset
            see above for details

        """
        xy = self.get_xy()
        z = self.get_z()
        inlrg, crlrg = self.ranges 
        info =  {
                    'comp': ['z'],
                    'iline': inlrg[0] if inlrg[0]==inlrg[1] else inlrg,
                    'xline': crlrg[0] if crlrg[0]==crlrg[1] else crlrg,
                    'x': xy[0],
                    'y': xy[1],
                }
        data = [z]
        allattribs = self.attribnames
        for attrib in attribnms:
            if attrib in allattribs:
                info['comp'].append(attrib)
                data.append( self.get_attrib(attrib))

        return self.to_xarray(data, info) if Horizon3D.use_xarray else (data, info,)

    def to_xarray(self, data: list[np.ndarray], info: dict):
        """Convert 3D horizon data in simple list+dict format to an Xarray Dataset

        See Horizon3D.getdata for details of the input and output formats.

        Parameters
        ----------
        data : list[np.ndarray]
           Z and any included horizon data, in iline, xline axis order
        info : dict
            at minimum require 'comp', 'iline, and 'xline' fields

        Returns
        -------
        Xarray.Dataset

        """

        from xarray import DataArray, Dataset
        si = self._survey.info()
        di = self.info()
        dims = ['iline', 'xline']
        xyattrs = {'units': si['xyunit']}
        inlrg = info['iline']
        crlrg = info['xline']
        coords =    {
                        'iline': np.arange(inlrg[0],inlrg[1]+inlrg[2],inlrg[2], dtype=np.int32),
                        'xline': np.arange(crlrg[0],crlrg[1]+crlrg[2],crlrg[2], dtype=np.int32),
                    }
        if set(("x","y")) <= info.keys():   
            coords['x'] = DataArray(info['x'], dims=dims, attrs=xyattrs)
            coords['y'] = DataArray(info['y'], dims=dims, attrs=xyattrs)

        attribs =   {
                        'description': di['name'],
                        'units': di['zunit'],
                        'crs': si['crs']
                    }
        return Dataset(data_vars={dv: (dims, data[idx]) for idx, dv in enumerate(info['comp'])}, coords=coords, attrs=attribs)

    def from_xarray(self, xrdata) ->tuple:
        """Convert 3D horizon data in Xarray.Dataset to simple list+dict format

        See Horizon3D.getdata for details of the input and output formats.
        Note:
        - only coordinates iline, xline need to be present in the dataset

        Parameters
        ----------
        xrdata : Xarray.Dataset

        Returns
        -------
        data : list[np.ndarrays], one array per "Z" and horizon data
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
        xl2 = 1 if xrdata['xline'].size==1 else round((xl1-xl0)/(data[0].shape[-1]-1))
        info['iline'] = [il0, il1, il2]
        info['xline'] = [xl0, xl1, xl2]
        info['comp'] = list(xrdata.data_vars)
        return (data, info,)

    def putdata(self, indata):
        """Write 3D horizon data from either simple list+dict format or Xarray.Dataset

        See Horizon3D.getdata for details of the input formats.
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
            see Horizon3D.getdata for details

        """
        data = []
        info = {}
        if isinstance(indata, tuple):
            data = indata[0]
            info = indata[1]
        else:
            data, info = self.from_xarray(indata)

        inlrg = info['iline']
        crlrg = info['xline']
        datashp = []
        if inlrg[0]!=inlrg[1]:
            datashp.append(((inlrg[1]-inlrg[0])//inlrg[2])+1)

        if crlrg[0]!=crlrg[1]:
            datashp.append(((crlrg[1]-crlrg[0])//crlrg[2])+1)

        for idx, compnm in enumerate(info['comp']):
            if idx<len(data):
                datum = data[idx]
            else:
                return

            if datum.shape!=tuple(datashp):
                raise ValueError(f'ranges {tuple(datashp)} and data shape {datum.shape} are incompatible for {compnm}.')

            datum = np.ascontiguousarray(datum, dtype=np.float32)
            if compnm=='z':
                self.put_z(datum, inlrg, crlrg)
            else:
                self.put_auxdata(compnm, datum, inlrg, crlrg)

            if not self.isok:
                raise ValueError(self.errmsg)



    def put_z(self, data, inlrg: list[int], crlrg: list[int]):
        """Save the 3D horizon Z values from the data Numpy 2D array

        Parameters
        ----------
        data : numpy.ndarray
            Horizon Z values
        inlrg : list[int]
            Inline start, stop and step
        crlrg : list[int]
            Crossline start, stop and step
        """

        npdata = data if isinstance(data, np.ndarray) else np.array(data)
        ct_inlrg = (ct.c_int * 3)(*inlrg)
        ct_crlrg = (ct.c_int * 3)(*crlrg)
        self._putz(self._handle, npdata, ct_inlrg, ct_crlrg)
        if not self.isok:
            raise ValueError(self.errmsg)

    def put_auxdata(self, name: str, data, inlrg: list[int], crlrg: list[int]):
        """Save the 3D horizon attribute data from a numpy 2D array

        Parameters
        ----------
        name : str
            Horizon attribute name
        data : numpy.ndarray
            Horizon sttribute values
        inlrg : list[int]
            Inline start, stop and step
        crlrg : list[int]
            Crossline start, stop and step
        """

        npdata = data if isinstance(data, np.ndarray) else np.array(data)
        ct_inlrg = (ct.c_int * 3)(*inlrg)
        ct_crlrg = (ct.c_int * 3)(*crlrg)
        self._putauxdata(self._handle, name.encode(), npdata, ct_inlrg, ct_crlrg)
        if not self.isok:
            raise ValueError(self.errmsg)

    def delete_attribs(self, attribnms: list[str]=[]):
        """Delete the listed attributes from the 3D horizon

        Parameters
        ----------
        attribnms : list[str]
            list of attribute names to be deleted

        Returns
        -------
        bool : True/False indicating success/failure

        """
        attribnmsptr = makestrlist(attribnms)
        res = self._delattribs(self._handle, attribnmsptr)
        stringset_del(attribnmsptr)
        return res

Horizon3D._initbindings('horizon3d')
 
