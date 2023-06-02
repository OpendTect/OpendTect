#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define basedata package variables
#

SET( LIBLIST omf ColTabs MouseControls BatchPrograms
	     FileFormats UnitsOfMeasure Properties odSettings welldispSettings
	     EnvVars ShortCuts *.ico *.png qtdefaultstate.ini RockPhysics ModDeps.od
	     Mnemonics )
set( DATADIRLIST Attribs BasicSurvey CRS icons.Default Strat Scripts )
set( PYTHONREQLIST
	"basic_requirements"
	"notebooks_requirements"
	"presentation_maker_requirements" )
SET( EXECLIST  )
SET( PACK "basedata")
