dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#This is the Final Script to run  all Scripts

Comment "------------Link file to all Scripts---------------------"

Case Insensitive

Menu "View`Toolbars`Graphical tools" On
#don't change order.

Include "$SCRIPTSDIR$/AddAttributetoTree.cmd"
Include "$SCRIPTSDIR$/EvalutionEnergyAttrib.cmd"

Include "$SCRIPTSDIR$/AllAttributes.cmd"
Include "$SCRIPTSDIR$/PickSet.cmd"

Include "$SCRIPTSDIR$/AttributeCrossplot.cmd" 
Include "$SCRIPTSDIR$/AttributeWellCrossplot.cmd"

Include "$SCRIPTSDIR$/SceneProperities.cmd"
Include "$SCRIPTSDIR$/Horizongrid.cmd"
Include "$SCRIPTSDIR$/HorizonSlice.cmd"
Include "$SCRIPTSDIR$/BetweenHorizon.cmd"
Include "$SCRIPTSDIR$/Horizon-Isopach.cmd"
Include "$SCRIPTSDIR$/HorizonFillHoles.cmd"
Include "$SCRIPTSDIR$/TreeitemHorizon.cmd"  //Add to cvs//
Include "$SCRIPTSDIR$/StratalAmp.cmd"

Include "$SCRIPTSDIR$/rgb-Array-Canvas.cmd"
Include "$SCRIPTSDIR$/ManageWavelet.cmd"
Include "$SCRIPTSDIR$/ChronoStratigraphy.cmd"
Include "$SCRIPTSDIR$/CreateChronoStratigraphy.cmd"
Include "$SCRIPTSDIR$/CreateSteeringCube.cmd"

Include "$SCRIPTSDIR$/ExportData.cmd"
#................Problem while Importing..(well properties).......
Include "$SCRIPTSDIR$/ImportData.cmd"
Include "$SCRIPTSDIR$/SEGY-Load.cmd"

Include "$SCRIPTSDIR$/CreateSeismicOutput.cmd"
Include "$SCRIPTSDIR$/CreateChimneyCube.cmd"
Include "$SCRIPTSDIR$/CreateFaultCube.cmd"
Include "$SCRIPTSDIR$/UVQ_waveformSegm.cmd"   //Add to cvs
Include "$SCRIPTSDIR$/GMT.cmd"   //Add to cvs
Include "$SCRIPTSDIR$/ImportSeismicAttributeSet.cmd"   //Add to cvs
Include "$SCRIPTSDIR$/ManageWells.cmd"  //Add to cvs

End

