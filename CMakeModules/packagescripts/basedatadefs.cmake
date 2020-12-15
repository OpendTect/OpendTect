#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define basedata package variables
# Author:       Nageswara
# Date:         September 2012
#RCS:           $Id$

SET( DATALIBLIST omf ColTabs MouseControls BatchPrograms
	     FileFormats UnitsOfMeasure Properties odSettings
	     EnvVars ShortCuts *.ico *.png RockPhysics ModDeps.od
	     prodlist.txt Vendors Mnemonics )
set( DATADIRLIST Attribs BasicSurvey icons.Default Strat
		 Python  Scripts CRS )
SET( EXECLIST  )
SET( PACK "basedata")