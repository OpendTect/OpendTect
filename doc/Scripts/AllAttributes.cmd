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

Include "/dsk/d12/nageswara/dev/od/doc/Scripts/coherency.cmd"
Include "/dsk/d12/nageswara/dev/od/doc/Scripts/convolve.cmd"
Include "/dsk/d12/nageswara/dev/od/doc/Scripts/curvature.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/dip.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/dipangle.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/energy.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/event.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/frequency.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/frequencyfilter.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/instantaneous.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/position.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/referenceshift.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/scaling.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/similarity.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/spectraldecomp.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/velocityfanfilter.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/volumestatistics.cmd"
Ok

Comment "--------Applying above Attributes on Tree items--------"

#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/Attrib_onInl.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/Attrib_onCrl.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/Attrib_onTimeSlice.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/Attrib_onVol.cmd"
Include "/dsk/d12/nageswara/dev/od/doc/Scripts/Attrib_onRandLine.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/PickSet.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/Attrib_onHorizon.cmd"
#Include "/dsk/d12/nageswara/dev/od/doc/Scripts/Attrib_onWell.cmd"

# Ready!!

End
