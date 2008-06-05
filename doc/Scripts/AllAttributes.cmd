dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#* Select attribute set 'Demo attributes'
Comment "----------Applying Attributes on all Tree items"

Case Insensitive

Menu "processing`Attributes"
Window "Attribute Set 3D"

Button "Save on OK" off
# Add all attributes
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/coherency.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/convolve.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/curvature.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/dip.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/dipangle.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/energy.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/event.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/frequency.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/frequencyfilter.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/instantaneous.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/position.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/referenceshift.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/scaling.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/similarity.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/spectraldecomp.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/velocityfanfilter.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/volumestatistics.cmd"
Ok

Include "/d12/nageswara/surveys/F3_Demo_new/Proc/Attrib_onInl.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/Attrib_onCrl.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/Attrib_obTimeSlice.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/Attrib_onVol.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/Attrib_obRandLine.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/pickSet.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/Attrib_onHorizon.cmd"
Include "/d12/nageswara/surveys/F3_Demo_new/Proc/Attri_onWell.cmd"

# Ready!!

End
