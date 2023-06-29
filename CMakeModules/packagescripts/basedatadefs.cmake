#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define basedata package variables
# Should only list files which are independent of the platform AND
# the cmake configuration
#

set( SPECFILES GNU_GENERAL_PUBLIC_LICENSE.txt INSTALL.txt LICENSE.txt )

set( DATALIST Attribs BasicSurvey CRS icons.Default Scripts Strat )
list( APPEND DATALIST BatchPrograms ColTabs EnvVars FileFormats Mnemonics
	ModDeps.od MouseControls odSettings omf qtdefaultstate.ini Properties
	RockPhysics ShortCuts UnitsOfMeasure welldispSettings )

file( GLOB DATAICONS RELATIVE "${CMAKE_INSTALL_PREFIX}/${OD_DATA_INSTALL_RELPATH}"
	"${CMAKE_INSTALL_PREFIX}/${OD_DATA_INSTALL_RELPATH}/*.ico"
	"${CMAKE_INSTALL_PREFIX}/${OD_DATA_INSTALL_RELPATH}/*.png" )
list( APPEND DATALIST ${DATAICONS} )

set( PYTHONREQLIST
	"basic_requirements"
	"notebooks_requirements"
	"presentation_maker_requirements" )

set( PACK basedata )
