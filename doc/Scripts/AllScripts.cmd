dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#This is the Final Script to run  all Scripts

Comment "------------Link file to all Scripts---------------------"

Case Insensitive

Menu "View`Toolbars`Graphical tools" On

Include "$SCRIPTSDIR$/AddAttributetoTree.cmd"
Include "$SCRIPTSDIR$/AllAttributes.cmd"
Include "$SCRIPTSDIR$/Attrib_onInl.cmd"
Include "$SCRIPTSDIR$/Attrib_onCrl.cmd"
Include "$SCRIPTSDIR$/Attrib_onTimeSlice.cmd"
Include "$SCRIPTSDIR$/Attrib_onVol.cmd"
Include "$SCRIPTSDIR$/Attrib_onRandLine.cmd"
Include "$SCRIPTSDIR$/PickSet.cmd"
Include "$SCRIPTSDIR$/Attrib_onHorizon.cmd"
Include "$SCRIPTSDIR$/Attrib_onWell.cmd"

#Deleting All attributes and adding coherencyAttribute.

Include "$SCRIPTSDIR$/AttributeCrossplot.cmd" 
Include "$SCRIPTSDIR$/AttributeWellCrossplot.cmd"

Include "$SCRIPTSDIR$/Horizongrid.cmd"
Include "$SCRIPTSDIR$/HorizonSlice.cmd"
Include "$SCRIPTSDIR$/BetweenHorizon.cmd"
Include "$SCRIPTSDIR$/ChronoStratigraphy.cmd"
Include "$SCRIPTSDIR$/CreateChronoStratigraphy.cmd"

Include "$SCRIPTSDIR$/ExportData.cmd"
Include "$SCRIPTSDIR$/ImportData.cmd"

End
