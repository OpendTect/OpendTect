dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#Defining All Attributes and Adding to List
Comment "---------Includeing of atready defined attributes --------------"

Include "$SCRIPTSDIR$/AttributeSet3DWindowPrepare.cmd"
Include "$SCRIPTSDIR$/coherency.cmd"
Include "$SCRIPTSDIR$/convolve.cmd"
Include "$SCRIPTSDIR$/curvature.cmd"
Include "$SCRIPTSDIR$/dip.cmd"
Include "$SCRIPTSDIR$/dipangle.cmd"
Include "$SCRIPTSDIR$/energy.cmd"
Include "$SCRIPTSDIR$/event.cmd"
Include "$SCRIPTSDIR$/frequency.cmd"
Include "$SCRIPTSDIR$/frequencyfilter.cmd"
Include "$SCRIPTSDIR$/gapdecon.cmd"
Include "$SCRIPTSDIR$/instantaneous.cmd"
Include "$SCRIPTSDIR$/position.cmd"
Include "$SCRIPTSDIR$/referenceshift.cmd"
Include "$SCRIPTSDIR$/scaling.cmd"
Include "$SCRIPTSDIR$/similarity.cmd"
Include "$SCRIPTSDIR$/spectraldecomp.cmd"
Include "$SCRIPTSDIR$/velocityfanfilter.cmd"
Include "$SCRIPTSDIR$/volumestatistics.cmd"

Button "Save attribute set"
Input "Name" "DemoAttributes_for_Scripts"
Ok
Ok

#Include "$SCRIPTSDIR$/Attrib_onInl.cmd"
#Include "$SCRIPTSDIR$/Attrib_onCrl.cmd"
#Include "$SCRIPTSDIR$/Attrib_onTimeSlice.cmd"
#Include "$SCRIPTSDIR$/Attrib_onVol.cmd"
#Include "$SCRIPTSDIR$/Attrib_onRandLine.cmd"
#Include "$SCRIPTSDIR$/Attrib_onHorizon.cmd"
Include "$SCRIPTSDIR$/Attrib_onWell.cmd"

Menu "Analysis`Attributes"
Window "Attribute Set*"
Button "Open attribute set"
ListClick "Objects list" "DemoAttributes_for_Scripts"
Button "Remove this object"
Button "Remove"
Button "Cancel"
Ok
Menu "Survey`Select/Setup"
Window "Survey selection"
Ok

End

