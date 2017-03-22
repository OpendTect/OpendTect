#!/bin/csh -f
#
# MATLAB environment
#


# Preconditions: bindir and MATLAB_DIR should be healthy

if ( $?bindir == 0 ) then
    echo "$0 should be run by the 'init_dtect' script"
    exit 1
else if ( ! -d "${bindir}" ) then
    echo "Cannot locate release binaries directory: ${bindir}"
    exit 1
endif

if ( ! $?MATLAB_DIR ) then
    exit 1
else if ( ! -d "${MATLAB_DIR}" ) then
    if ( -e "${MATLAB_DIR}" ) then
	echo "MATLAB_DIR (${MATLAB_DIR}) directory does not exist"
    else
	echo "MATLAB_DIR (${MATLAB_DIR}) is not a proper directory"
    endif
    exit 1
endif


# OK we're cool!


# Default location of the compiled shared libraries:
if ( $?MATLAB_BUILDDIR == 0 ) then
    setenv MATLAB_BUILDDIR "${MATLAB_DIR}"
else if ( ! -d "${MATLAB_BUILDDIR}" ) then
    setenv MATLAB_BUILDDIR "${MATLAB_DIR}"
endif


setenv MATLAB_LD_LIBRARY_PATH "${MATLAB_DIR}"/bin/glnxa64:"${MATLAB_DIR}"/runtime/glnxa64
if ( ! $?LD_LIBRARY_PATH ) then
    setenv LD_LIBRARY_PATH "${MATLAB_LD_LIBRARY_PATH}"
else
    setenv LD_LIBRARY_PATH "${MATLAB_LD_LIBRARY_PATH}:${LD_LIBRARY_PATH}"
endif
