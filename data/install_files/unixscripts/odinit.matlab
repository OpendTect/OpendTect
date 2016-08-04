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

if ( -f "${bindir}/libstdc++.so.6" || -f "${bindir}/libgcc_s.so.1" ) then
    if ( -w "${bindir}" ) then
	if ( -e "${bindir}"/libstdc++.so.6 ) then
	    mv "${bindir}"/libstdc++.so.6 ${bindir}/libstdc++_BU.so.6
	endif
	if ( -e "${bindir}"/libgcc_s.so.1 ) then
	    mv "${bindir}"/libgcc_s.so.1 ${bindir}/libgcc_s_BU.so.1
	endif
    else
	echo ""
	echo "The following libraries are installed by OpendTect:"
	echo ""
	echo " in ${bindir} :"
	echo "      libstdc++.so.6"
	echo "      libgcc_s.so.1"
	echo ""
	echo "these are not compatible with the MATLAB libraries."
	echo "Please ask your system administrator to rename or move away these files"
	echo "in order to use the OpendTect-MATLAB plugin "
	echo ""
    endif
endif
