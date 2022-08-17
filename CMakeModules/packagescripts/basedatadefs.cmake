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
	     EnvVars ShortCuts *.ico *.png RockPhysics ModDeps.od
	     Mnemonics )
set( DATADIRLIST Attribs BasicSurvey icons.Default Strat
		 Python  Scripts CRS )
SET( EXECLIST  )
SET( PACK "basedata")
