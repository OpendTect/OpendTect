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
        clss._getzidx = wrap_function(LIBODB, f'{bindnm}_getzidx', ct.c_int, [ct.c_void_p, ct.c_float])
        clss._getzval = wrap_function(LIBODB, f'{bindnm}_getzval', ct.c_float, [ct.c_void_p, ct.c_int])
        clss._gettrcidx = wrap_function(LIBODB, f'{bindnm}_gettrcidx', ct.c_int, [ct.c_void_p, ct.c_int, ct.c_int])
        clss._getinlcrl = wrap_function(LIBODB, f'{bindnm}_getinlcrl', None, [ct.c_void_p, ct.c_int, ct.POINTER(ct.c_int), ct.POINTER(ct.c_int)])
        clss._getdata = wrap_function(LIBODB, f'{bindnm}_getdata', None, [ct.c_void_p, NumpyAllocator.CFUNCTYPE,
                                                                            ct.POINTER(ct.c_int), ct.POINTER(ct.c_int), ct.POINTER(ct.c_int)                    
                                                                        ])

    @property
    def bin_count(self) ->int:
        """int : Number of bins within the extent of this seismic volume, number_of_bins>=number_of_traces"""
        return self._nrbins(self._handle)

    @property
    def comp_names(self) ->list[str]:
        """list[str]: Names of components in this seismic volume (readonly)"""
        return pystrlist(self._compnames(self._handle))

    @property
    def trace_count(self) ->int:
        """int : Expected number of traces in this seismic volume."""
        return self._nrtrcs(self._handle)

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
        if not self._isok(self._handle):
            raise IndexError(pystr(self._errmsg(self._handle)))

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
        if not self._isok(self._handle):
            raise IndexError(f'(pystr(self._errmsg(self._handle)) {zidx}')

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
        if not self._isok(self._handle):
            raise IndexError(self._errmsg(self._handle))

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
        if not self._isok(self._handle):
            raise IndexError(self._errmsg(self._handle))

        return (inline.value, crline.value)

    def __getitem__(self, idx):
        """Either [trace_number] or [inl, crl] or [inl slice, crl slice, z slice]

        [trace_number] returns a single trace
        [inl, crl] return the single trace at the given iln, crl location
        [inl slice, crl slice, z slice] return a data volume. Examples:
            [200, :, :] returns inline 200, all traces and all z range
            [:, 300, :] returns crossline 300, all traces and all z range
            [200:300,400:450,500:600] returns a volume of 100 inlines, 50 crosslines and 100 z samples 

        Parameters
        ----------
        idx : int | (int, int) | (slice, slice, slice)
        
        Returns
        -------
        dict : with the trace data and supporting information

        """
        this_info = self.info()
        inlrg =  this_info['inl_range']
        crlrg =  this_info['crl_range']
        zrg =  [0, this_info['nrsamp']-1, 1]
        inls = None
        crls = None
        zs = None
        dims = []
        if isinstance(idx, int):
            inl, crl = self.bin(idx)
            inls = slice(inl, inl, 1)
            crls = slice(crl, crl, 1)
            dims.append('z')
        elif isinstance(idx, tuple) and len(idx)==2 and isinstance(idx[0], int) and isinstance(idx[1], int):
            inl, crl = idx
            inls = slice(inl, inl, 1)
            crls = slice(crl, crl, 1)
            dims.append('z')
        elif isinstance(idx, tuple) and len(idx)==3:
            inls, crls, zs = idx
            if isinstance(inls, int):
                inls = slice(inls, inls, 1)
            else:
                dims.append('inl')

            if isinstance(crls, int):
                crls = slice(crls, crls, 1)
            else:
                dims.append('crl')

            if isinstance(zs, int):
                zs = slice(zs, zs, 1)
            else:
                dims.append('z')

        else:
            raise IndexError('index should be either [trc_number], [inl, crl] or [inl slice, crl slice, z slice]')

        inlrg = inlrg if not inls else [inlrg[0] if not inls.start else inls.start, 
                                        inlrg[1] if not inls.stop else inls.stop, 
                                        inlrg[2] if not inls.step else inls.step]
        crlrg = crlrg if not crls else [crlrg[0] if not crls.start else crls.start,
                                        crlrg[1] if not crls.stop else crls.stop, 
                                        crlrg[2] if not crls.step else crls.step]
        zrg = zrg if not zs else [  zrg[0] if not zs.start else zs.start,
                                    zrg[1] if not zs.stop else zs.stop,
                                    zrg[2] if not zs.step else zs.step]
        allocator = NumpyAllocator()
        ct_inlrg = (ct.c_int * 3)(*inlrg)
        ct_crlrg = (ct.c_int * 3)(*crlrg)
        ct_zrg = (ct.c_int * 3)(*zrg)

        self._getdata(self._handle, allocator.cfunc, ct_inlrg, ct_crlrg, ct_zrg)
        if not self._isok(self._handle):
            raise ValueError(self._errmsg(self._handle))

        comps = self.comp_names
        data = { comp: allocator.allocated_arrays[comps.index(comp)] for comp in comps }
        data['comp'] = comps
        data['inl'] = inlrg[0] if inlrg[0]==inlrg[1] else [inl for inl in range(inlrg[0],inlrg[1]+inlrg[2],inlrg[2])]
        data['crl'] = crlrg[0] if crlrg[0]==crlrg[1] else [crl for crl in range(crlrg[0],crlrg[1]+crlrg[2],crlrg[2])]
        data['x'] = allocator.allocated_arrays[-2]
        data['y'] = allocator.allocated_arrays[-1]
        data['z'] = self.z_value(zrg[0]) if zrg[0]==zrg[1] else [self.z_value(z) for z in range(zrg[0],zrg[1]+zrg[2],zrg[2])]
        data['dims'] = dims
        return data

    def as_xarray(self, data: dict):
        from xarray import DataArray, Dataset
        name = self.info()['name']
        si = self._survey.info()
        dims_xy = [dim for dim in data['dims'] if dim!='z']
        xyattrs = {'units': si['xyunit']}
        zattrs = {'units': si['zunit']}
        coords =    {
                        'inl': data['inl'],
                        'crl': data['crl'],
                        'x': data['x'][0] if len(data['x'])==1 else DataArray(data['x'], dims=dims_xy, attrs=xyattrs),
                        'y': data['y'][0] if len(data['y'])==1 else DataArray(data['y'], dims=dims_xy, attrs=xyattrs),
                        'z': data['z'] if isinstance(data['z'],float) else DataArray(data['z'], dims=['z'], attrs=zattrs)
                    }
        attribs =   {
                        'description': name,
                        'units': si['zunit'],
                        'crs': si['crs']
                    }
        return Dataset(data_vars={dv: (data['dims'], data[dv]) for dv in data['comp']}, coords=coords, attrs=attribs)



Seismic3D._initbindings('seismic3d')
 
