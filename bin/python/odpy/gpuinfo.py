from numba import cuda as numcuda

def is_cuda_available():
  return numcuda.is_available()

def detect_gpus(printfn=print):
  printfn( numcuda.detect() )

def nr_gpus():
  return len(numcuda.gpus)

def compute_capability( devnum=0 ):
  return numcuda.gpus[devnum].compute_capability

def id( devnum=0 ):
  return numcuda.gpus[devnum].id

def name( devnum=0 ):
  return numcuda.gpus[devnum].name

def get_memory_info( devnum=None ):
  return numcuda.current_context(devnum=devnum).get_memory_info()
