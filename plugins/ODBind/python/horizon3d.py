import numpy as np
import ctypes as ct
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
        putzargs = [ct.c_void_p, 
                    np.ctypeslib.ndpointer(dtype=np.uint32, ndim=1, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.float32, ndim=2, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags="C_CONTIGUOUS"),
                    np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags="C_CONTIGUOUS")
                    ]
        clss._putz = wrap_function(LIBODB, f'{bindnm}_putz', None, putzargs)

    @property
    def attribnames(self) ->list[str]:
        """list[str]: Names of attributes attached to this 3D horizon (readonly)"""
        return pystrlist(self._attribnames(self._handle))

    @property
    def ilines(self) ->list[int]:
        """list[int]: Inline numbers included in this 3D horizon (readonly)"""
        hi_inl = self.info()['inl_range']
        inlrg = range(hi_inl[0], hi_inl[1]+hi_inl[2], hi_inl[2] )
        return [inl for inl in inlrg]

    @property
    def xlines(self) ->list[int]:
        """list[int]: Crossline numbers included in this 3D horizon (readonly)"""
        hi_crl = self.info()['crl_range']
        crlrg = range(hi_crl[0], hi_crl[1]+hi_crl[2], hi_crl[2] )
        return [crl for crl in crlrg]

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

    def getz(self):
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
     
    def getxy(self):
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

    def get_xarray(self):
        """Get the 3D horizon Z values as an XArray DataArray

        Returns
        -------
        XArray DataArray with the horizon Z values and both inline/crossline and X/Y coordinates

        """

        from xarray import DataArray
        name = self.info()['name']
        xy = self.getxy()
        z = self.getz()
        si = self._survey.info()
        dims = ['inl', 'crl']
        xyattrs = { 'units': si['xyunit']}
        coords =    {
                        'inl': self.ilines,
                        'crl': self.xlines,
                        'x': DataArray(xy[0], dims=dims, attrs=xyattrs),
                        'y': DataArray(xy[1], dims=dims, attrs=xyattrs)
                    }
        attribs =   {
                        'description': name,
                        'units': si['zunit'],
                        'crs': si['crs']
                    }
        return DataArray(z, coords=coords, dims=dims, name=name, attrs=attribs)

    def putz(self, data, inlines, crlines):
        """Save the 3D horizon Z values from the data Numpy 2D array

        Parameters
        ----------
        data : numpy.ndarray
            Horizon Z values
        inlines : arraylike
            Inline locations of Z values
        crlines : arraylike
            Crossline locations of Z values
        """

        npdata = data if isinstance(data, np.ndarray) else np.array(data)
        npinlines = inlines if isinstance(inlines, np.ndarray) else np.array(inlines, dtype=np.int32)
        npcrlines = crlines if isinstance(crlines, np.ndarray) else np.array(crlines, dtype=np.int32)
        shape = np.array(npdata.shape, dtype=np.uint32)
        self._putz(self._handle, shape, npdata, npinlines, npcrlines)
        if not self.isok:
            raise ValueError(self.errmsg)

    def put_xarray(self, data_array):
        """Save the 3D horizon Z values from the data_array XArray DataArray

        Parameters
        ----------
        data_array : XArray DataArray
            Horizon Z values and 'inl' and 'crl' coordinates
        """

        npdata = data_array.values
        npinlines = data_array.coords['inl'].to_numpy().astype(dtype=np.int32)
        npcrlines = data_array.coords['crl'].to_numpy().astype(dtype=np.int32)
        shape = np.array(npdata.shape, dtype=np.uint32)
        self._putz(self._handle, shape, npdata, npinlines, npcrlines)
        if not self.isok:
            raise ValueError(self.errmsg)


Horizon3D._initbindings('horizon3d')
 
