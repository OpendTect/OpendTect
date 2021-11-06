"""
Copyright (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  * AUTHOR : A. Huck
  * DATE   : Nov 2018

Module Summary
###############

Tools for gpu and cuda operations

"""

from numba import cuda as numcuda

def is_cuda_available():
  """ Checks if cuda device is available

  Returns:
    * bool: True if cuda is available, False if otherwise
  """

  return numcuda.is_available()

def detect_gpus(printfn=print):
  """ Displays cuda devices available

  Notes:
    Information on device type, id, compute type and capacity are displayed

  >>> import odpy.gpuinfo as gpuinfo
  >>> gpuinfo.detect_gpus()
      Found 1 CUDA devices
      id 0    b'NVIDIA GeForce MX250'                              [SUPPORTED]
                            compute capability: 6.1
                                pci device id: 0
                                    pci bus id: 2

  """

  printfn( numcuda.detect() )

def nr_gpus():
  """ Counts number of gpu devices

  Returns:
    int: number of gpu devices available for computations
  """

  return len(numcuda.gpus)

def compute_capability( devnum=0 ):
  """ Compute capacity of cuda device

  Parameters:
    * devnum (int): cuda device id, defaults to 0

  Returns:
    * tuple: compute capacity level of cuda device

  >>> import odpy.gpuinfo as gpuinfo
  >>> gpuinfo.compute_capability(0)
      (6, 1)
  """

  return numcuda.gpus[devnum].compute_capability

def id( devnum=0 ):
  """ Gets cuda device id

  Parameters:
    * devnum (int): cuda device number, defaults to 0

  Returns:
    * int: cuda device id
  """

  return numcuda.gpus[devnum].id

def name( devnum=0 ):
  """ Gets cuda device name

  Parameters:
    * devnum (int): cuda device number, defaults to 0

  Returns:
    * str: cuda device name
  """

  return numcuda.gpus[devnum].name

def get_memory_info( devnum=None ):
  """ Gets memory info of cuda device

  Parameters:
    * devnum (int): cuda device number, defaults to None

  Returns:
    * obj: Memory info 

  Example

  >>> import odpy.gpuinfo as gpuinfo
  >>> gpuinfo.get_memory_info()
      MemoryInfo(free=1742539571, total=2147483648)

  """

  return numcuda.current_context(devnum=devnum).get_memory_info()
