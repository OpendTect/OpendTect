#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to define basedata package variables
# Author:       Nageswara
# Date:         September 2012
#RCS:           $Id$

SET( LIBLIST omf Attribs BasicSurvey ColTabs MouseControls BatchPrograms
	     BatchHosts_example FileFormats UnitsOfMeasure Properties odSettings
	     EnvVars ShortCuts *.ico *.png *.html icons.Default
	     RockPhysics ModDeps.od Strat Python
	     prodlist.txt Vendors
	     BatchHosts_example_for_nodes_of_a_linux_server.txt
	     BatchHosts_example_for_nodes_of_a_windows_server.txt Scripts CRS )
SET( EXECLIST  )
SET( PACK "basedata")
