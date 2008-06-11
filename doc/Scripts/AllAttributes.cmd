dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#* Select attribute set 'Demo attributes'
Comment "----------Applying Attributes on all Tree items"

Case Insensitive

Menu "processing`Attributes"
Window "Attribute Set 3D"
Button "New attribute set" 
OnError Continue
Button "No"
OnError Stop
Combo "Attribute group" "<All>"
Button "Save on OK" off
# Add all attributes

Include "/$SCRIPTSDIR$/coherency.cmd"
Include "/$SCRIPTSDIR$/convolve.cmd"
Include "/$SCRIPTSDIR$/curvature.cmd"
Include "/$SCRIPTSDIR$/dip.cmd"
Include "/$SCRIPTSDIR$/dipangle.cmd"
Include "/$SCRIPTSDIR$/energy.cmd"
Include "/$SCRIPTSDIR$/event.cmd"
Include "/$SCRIPTSDIR$/frequency.cmd"
Include "/$SCRIPTSDIR$/frequencyfilter.cmd"
Include "/$SCRIPTSDIR$/instantaneous.cmd"
Include "/$SCRIPTSDIR$/position.cmd"
Include "/$SCRIPTSDIR$/referenceshift.cmd"
Include "/$SCRIPTSDIR$/scaling.cmd"
Include "/$SCRIPTSDIR$/similarity.cmd"
Include "/$SCRIPTSDIR$/spectraldecomp.cmd"
Include "/$SCRIPTSDIR$/velocityfanfilter.cmd"
Include "/$SCRIPTSDIR$/volumestatistics.cmd"
Ok

Comment "--------Applying above Attributes on Tree items--------"

Include "/$SCRIPTSDIR$/Attrib_onInl.cmd"
Include "/$SCRIPTSDIR$/Attrib_onCrl.cmd"
Include "/$SCRIPTSDIR$/Attrib_onTimeSlice.cmd"
Include "/$SCRIPTSDIR$/Attrib_onVol.cmd"
Include "/$SCRIPTSDIR$/Attrib_onRandLine.cmd"
Include "/$SCRIPTSDIR$/PickSet.cmd"
Include "/$SCRIPTSDIR$/Attrib_onHorizon.cmd"
Include "/$SCRIPTSDIR$/Attrib_onWell.cmd"

# Ready!!

End
