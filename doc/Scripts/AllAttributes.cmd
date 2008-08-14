dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

# Select attribute set 'Demo attributes'
Comment "----------Applying Attributes on all Tree items"

Case Insensitive

Menu "processing`Attributes"
Window "Attribute Set*"
Button "New attribute set" 
Combo "Attribute group" "<All>"
Button "Save on OK" off

#Defining All Attributes and Adding to List
Comment "---------Defining All Attributes and Adding to List--------------"

Include "$SCRIPTSDIR$/coherency.cmd"
Include "$SCRIPTSDIR$/convolve.cmd"
Include "$SCRIPTSDIR$/curvature.cmd"
Include "$SCRIPTSDIR$/dip.cmd"
Include "$SCRIPTSDIR$/dipangle.cmd"
Include "$SCRIPTSDIR$/energy.cmd"
Include "$SCRIPTSDIR$/event.cmd"
Include "$SCRIPTSDIR$/frequency.cmd"
Include "$SCRIPTSDIR$/frequencyfilter.cmd"
Include "$SCRIPTSDIR$/instantaneous.cmd"
Include "$SCRIPTSDIR$/position.cmd"
Include "$SCRIPTSDIR$/referenceshift.cmd"
Include "$SCRIPTSDIR$/scaling.cmd"
Include "$SCRIPTSDIR$/similarity.cmd"
Include "$SCRIPTSDIR$/spectraldecomp.cmd"
Include "$SCRIPTSDIR$/velocityfanfilter.cmd"
Include "$SCRIPTSDIR$/volumestatistics.cmd"
Ok

End
