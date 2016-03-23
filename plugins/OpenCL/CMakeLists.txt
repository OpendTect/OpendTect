#_______________________Pmake___________________________________________________
#
#	Makefile : 	OpenCL adaptions
# 	Mar 2016	K. Tingdahl
#_______________________________________________________________________________

if ( OpenCL_FOUND )
    set( OD_IS_PLUGIN TRUE )
    set( OD_MODULE_DEPS Basic )

    set( OD_TEST_PROGS
	opencl.cc
    )

    set( OD_MODULE_SOURCES
	openclplatform.cc
	openclpi.cc
    )

    OD_SETUP_OPENCL()

    OD_INIT_MODULE()
endif( OpenCL_FOUND )
