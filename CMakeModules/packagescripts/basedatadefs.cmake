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

set( DATALIST Attribs BasicSurvey CRS icons.Default Strat )
list( APPEND DATALIST BatchPrograms ColTabs EnvVars FileFormats Mnemonics
	ModDeps.od MouseControls odSettings omf qtdefaultstate.ini Properties
	RockPhysics ShortCuts UnitsOfMeasure welldispSettings )

file( GLOB DATAICONS RELATIVE "${COPYFROMDATADIR}"
	"${COPYFROMDATADIR}/*.ico"
	"${COPYFROMDATADIR}/*.png" )
list( APPEND DATALIST ${DATAICONS} )

list( APPEND OTHERFILES "${COPYFROMDATADIR}/localizations/od_en_US.qm" )
list( APPEND OTHERFILESDEST "${COPYTODATADIR}/localizations" )

list( APPEND OTHERFILES "${COPYFROMDIR}/relinfo/README.txt" )
list( APPEND OTHERFILESDEST "${COPYTODIR}/relinfo" )
list( APPEND OTHERFILES "${COPYFROMDIR}/relinfo/RELEASEINFO.txt" )
list( APPEND OTHERFILESDEST "${COPYTODIR}/doc/ReleaseInfo" )

list( APPEND OTHERFILES "${COPYFROMDIR}/doc/Videos.od" )
list( APPEND OTHERFILESDEST "${COPYTODIR}/doc" )

set( PYTHONREQLIST
	"basic_requirements"
	"notebooks_requirements"
	"presentation_maker_requirements" )

set( ISBASE TRUE )

set( PACK basedata )
