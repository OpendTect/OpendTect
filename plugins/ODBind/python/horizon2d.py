import numpy as np
import ctypes as ct
from odbind import wrap_function, LIBODB, makestrlist, NumpyAllocator, pyjsonstr, pystr, pystrlist, stringset_del 
from odbind.survey import Survey, _SurveyObject


class Horizon2D(_SurveyObject):
    """
    A class for an OpendTect 2D horizon
    """

    @classmethod
    def _initbindings(clss, bindnm):
        clss._initbasebindings(bindnm)
        clss._attribnames = wrap_function(LIBODB, f'{bindnm}_attribnames', ct.c_void_p, [ct.c_void_p])
        clss._linecount = wrap_function(LIBODB, f'{bindnm}_linecount', ct.c_int, [ct.c_void_p])
        clss._lineids = wrap_function(LIBODB, f'{bindnm}_lineids', None, [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_int)])
        clss._linename = wrap_function(LIBODB, f'{bindnm}_linename', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_int])
        clss._linenames = wrap_function(LIBODB, f'{bindnm}_linenames', ct.c_void_p, [ct.c_void_p])
        clss._getz = wrap_function(LIBODB, f'{bindnm}_getz', ct.c_void_p, [ct.c_void_p, NumpyAllocator.CFUNCTYPE, ct.c_int])
        clss._getxy = wrap_function(LIBODB, f'{bindnm}_getxy', ct.c_void_p, [ct.c_void_p, NumpyAllocator.CFUNCTYPE, ct.c_int])


    @property
    def attribnames(self) ->list[str]:
        """list[str]: Names of attributes attached to this 2D horizon (readonly)"""
        return pystrlist(self._attribnames(self._handle))

    def lineids(self) ->list[int]:
        """ Return the OpendTect line indices of 2D lines in this 2D horizon.

        Returns
        -------
        list[int]

        """

        ids = (ct.c_int * self._linecount(self._handle))()
        self._lineids(self._handle, len(ids), ids)
        return ids[:]

    def linename(self, lineid) ->str:
        """ Return the line name of the 2D line with the given lineid.

        Returns
        -------
        str

        """

        return pystr(self._linename(self._handle, lineid))

    def linenames(self) ->list[str]:
        """ Return the line names of 2D lines in this 2D horizon.

        Returns
        -------
        list[str]

        """

        return pystrlist(self._linenames(self._handle))

    def getz(self, lineid):
        """Get the 2 horizon Z values for the given lineid as a Numpy array

        Returns
        -------
        Numpy 1D array with the horizon Z values for the given lineid

        """

        allocator = NumpyAllocator()
        self._getz(self._handle, allocator.cfunc, lineid)
        if not self._isok(self._handle):
            raise ValueError(self._errmsg(self._handle))

        return allocator.allocated_arrays[0]
     
    def getxy(self, lineid):
        """Get the 2D horizon X,Y and trace number values for the given lineid as Numpy arrays

        Returns
        -------
        Tuple of Numpy 1D arrays with the horizon X, Y, trace number values for the given lineid

        """

        allocator = NumpyAllocator()
        self._getxy(self._handle, allocator.cfunc, lineid)
        if not self._isok(self._handle):
            raise ValueError(self._errmsg(self._handle))

        return (allocator.allocated_arrays[:3])

    def get_xarray(self, lineid):
        """Get the 2D horizon Z values for the given lineid as an XArray DataArray

        Returns
        -------
        XArray DataArray with the horizon Z values for the given lineid and trace numbers and X/Y coordinates

        """

        from xarray import DataArray
        name = self.info()['name']
        xy = self.getxy(lineid)
        z = self.getz(lineid)
        si = self._survey.info()
        dims = ['trc']
        xyattrs = { 'units': si['xyunit']}
        coords =    {
                        'lineid': lineid,
                        'trc': xy[2],
                        'linename': self.linename(lineid),
                        'x': DataArray(xy[0], dims=dims, attrs=xyattrs),
                        'y': DataArray(xy[1], dims=dims, attrs=xyattrs)
                    }
        descname = name + ':' + coords['linename']
        attribs =   {
                        'description': descname,
                        'units': si['zunit'],
                        'crs': si['crs']
                    }
        return DataArray(z, coords=coords, dims=dims, name=descname, attrs=attribs)



Horizon2D._initbindings('horizon2d')

 
