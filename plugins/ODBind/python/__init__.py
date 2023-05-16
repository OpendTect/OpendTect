__version__ = '1.0.0'

"""Python CTypes bindings to OpendTect

Copyright (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt

Module Summary
###############

The OpendTect C++ ODBind plugin provides an FFI (foreign function interface) compatible interface to data within 
an OpendTect survey/project. The "odbind" Python module is a Python ctypes binding and object oriented wrapper 
to the ODBind plugin.


Python Classes
-------------
Survey
    Encapsulates an OpendTect survey/project, providing information about the survey. Survey objects are used as context
    to construct the various data objects. The Survey class also includes a number of static class methods that provide
    information about all or a user speciifed subset of the surveys within a specific OpendTect data folder.

Horizon2D
    Encapsulates an OpendTect 2D seismic horizon.

Horizon3D
    Encapsulates an OpendTect 3D seismic horizon.

Seismic2D
    Encapsulates an OpendTect 2D seismic dataset.

Seismic3D
    Encapsulates an OpendTect 3D seismic dataset.

Well
    Encapsulates an OpendTect well

Examples
--------

"""
import sys, os
import ctypes as ct
import json
import atexit
import numpy as np
import odpy.common as odc

def get_lib_name(modnm: str) -> str:
    ''' Get platform dependent shared library name for given module '''
    libnm = str()
    if sys.platform == 'win32':
        libnm = modnm + '.dll'
    else:
        libnm = 'lib' + modnm
        if sys.platform == 'darwin':
            libnm = libnm + '.dylib'
        else:
            libnm = libnm + '.so'
    
    return libnm

def wrap_function(lib, funcname, restype, argtypes):
    ''' Simplify wrapping ctypes functions '''
    func = lib.__getattr__(funcname)
    func.restype = restype
    func.argtypes = argtypes
    return func

class NumpyAllocator:
    CFUNCTYPE = ct.CFUNCTYPE(ct.c_long, ct.c_int, ct.POINTER(ct.c_int), ct.c_char)
    _dtype = {  'i': np.dtype('i4'),
                'f': np.dtype('f4'),
                'd': np.dtype('f8')
             }

    def __init__(self):
        self.allocated_arrays = []

    def __call__(self, dims, shape, dtype):
        x = np.empty(shape[:dims], self._dtype[dtype.decode()])
        self.allocated_arrays.append(x)
        return x.ctypes.data_as(ct.c_void_p).value

    def getcfunc(self):
        return self.CFUNCTYPE(self)
    cfunc = property(getcfunc)

libodbind = os.path.join(odc.getExecPlfDir(), get_lib_name('ODBind'))
LIBODB = ct.CDLL(libodbind)
init_module = wrap_function(LIBODB, 'initModule', None, [ct.c_char_p])
exit_module = wrap_function(LIBODB, 'exitModule', None, [])
init_module(libodbind.encode())
atexit.register(exit_module)

cstring_del = wrap_function(LIBODB, 'cstring_del', None, [ct.POINTER(ct.c_char_p)])
_getdatadir = wrap_function(LIBODB, 'getUserDataDir', ct.POINTER(ct.c_char_p), [])
_getsurvey = wrap_function(LIBODB, 'getUserSurvey', ct.POINTER(ct.c_char_p), [])

def pystr(cstringptr: ct.POINTER(ct.c_char_p), autodel: bool=True) ->str:
    """Convert char* to Python string with optional auto deletion of input char*

    Parameters
    ----------
    cstringptr : ctypes.POINTER(ctypes.c_char_p)
        the char* to convert
    autodel : bool=True
        (Optional) if True delete the input pointer after conversion

    Returns
    -------
    str : the contents of the input pointer as a Python string

    """
    pystring = ct.cast(cstringptr, ct.c_char_p).value.decode()
    if autodel:
        cstring_del(cstringptr)
    return pystring

def get_user_datadir() ->str:
    """Get the OpendTect data directory/folder from the user's OpendTect settings

    Returns
    -------
    str : the OpendTect data directory from the user's OpendTect settings

    """
    return pystr(_getdatadir())

def get_user_survey() ->str:
    """Get the current OpendTect survey from the user's OpendTect settings

    Returns
    -------
    str : the current OpendTect survey from the user's OpendTect settings

    """
    return pystr(_getsurvey())

stringset_new = wrap_function(LIBODB, 'stringset_new', ct.c_void_p, [])
stringset_copy = wrap_function(LIBODB, 'stringset_copy', ct.c_void_p, [ct.c_void_p])
stringset_del = wrap_function(LIBODB, 'stringset_del', None, [ct.c_void_p])
stringset_size = wrap_function(LIBODB, 'stringset_size', ct.c_int, [ct.c_void_p])
stringset_add = wrap_function(LIBODB, 'stringset_add', ct.c_void_p, [ct.c_void_p, ct.c_char_p])
stringset_get = wrap_function(LIBODB, 'stringset_get', ct.POINTER(ct.c_char_p), [ct.c_void_p, ct.c_int])
def makestrlist(inlist: list[str]) ->ct.c_void_p:
    """Make a BufferStringSet from a Python list

    Parameters
    ----------
    inlist : list
        Input Python list
    
    Returns
    -------
    ctypes.c_void_p : a pointer to the new BufferStringSet

    """
    res = stringset_new()
    for val in inlist:
        stringset_add(res, val.encode())
    return res

def pystrlist(stringsetptr: ct.c_void_p, autodel: bool=True) ->list[str]:
    """Convert a BufferStringSet* to a Python list with optional automatic deletion

    Parameters
    ----------
    stringsetptr : ctypes.c_void_p
        the BufferStringSet* to convert
    autodel : bool=True
        (Optional) if True delete the input pointer after conversion

    Returns
    -------
    list[str] : contents of the input pointer as a Python list of strings

    """
    res = []
    for idx in range(stringset_size(stringsetptr)):
        res.append(pystr(stringset_get(stringsetptr, idx), False))
    if autodel:
        stringset_del(stringsetptr)
    return res

def pyjsonstr(jsonstrptr: ct.POINTER(ct.c_char_p), autodel: bool=True):
    """Convert a char* JSON string to a Python object with optional automatic deletion

    Parameters
    ----------
    jsonstrptr : ctypes.POINTER(ctype.c_char_p)
        the input JSON char* to convert
    autodel : bool=True
        (Optional) if True delete the input pointer after conversion

    Returns
    -------
    list/dict : contents of the input pointer as a Python list/dict

    """
    res = json.loads(ct.cast(jsonstrptr, ct.c_char_p).value.decode())
    if autodel:
        cstring_del(jsonstrptr)
    return res

def unpack_slice(rgs: slice, rg: list[int]) ->list[int]:
    """Unpack a slice optionally replacing missing values using contents of rg"""
    outrg = rg if not rgs else [rg[0] if not rgs.start else rgs.start, 
                                rg[1] if not rgs.stop else rgs.stop, 
                                rg[2] if not rgs.step else rgs.step]
    return outrg

def is_none_slice(x: slice) ->bool:
    """Return True if all slice components are None"""
    return x==slice(None, None, None)

__all__ = ['survey', 'horizon2d', 'horizon3d', 'well']