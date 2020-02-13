import numba

def is_cuda_available():
  return numba.cuda.is_available()

def detect_gpus(printfn=print):
  printfn( numba.cuda.detect() )

def nr_gpus():
  return len(numba.cuda.gpus)

def compute_capability( devnum=0 ):
  return numba.cuda.gpus[devnum].compute_capability

def id( devnum=0 ):
  return numba.cuda.gpus[devnum].id

def name( devnum=0 ):
  return numba.cuda.gpus[devnum].name

def get_memory_info( devnum=None ):
  return numba.cuda.current_context(devnum=devnum).get_memory_info()
