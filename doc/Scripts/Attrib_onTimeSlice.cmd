dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Wheel "vRotate" 90

#Assining Values to Timeslice number 400
Comment "--------Assining Values to Timeslice number 400-----------"
TreeMenu "Timeslice" "Add"
TreeMenu "Timeslice`924" "Position"
window "Positioning"
Input "Inl Start" 160
Input "Inl Stop" 350
Input "Crl Start" 850
Input "Crl Stop" 1150
Input "Z" 400
Ok
ListClick "Select Data" "CoherencyAttrib"
Ok
Button "Make snapshot"
Input "Select filename" "ts_400_coherencyatt.jpg"
Ok
TreeButton "Timeslice`400`CoherencyAttr*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_convolveatt.jpg"
Ok
TreeButton "Timeslice`400`ConvolveAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_curvatureatt.jpg"
Ok
TreeButton "Timeslice`400`CurvatureAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_dipatt.jpg"
Ok
TreeButton "Timeslice`400`DipAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_dipangleatt.jpg"
Ok
TreeButton "Timeslice`400`DipAngleAtt*" Off

TreeMenu "Timeslice`400`CoherencyAttr*" "Remove"
TreeMenu "Timeslice`400`ConvolveAttr*" "Remove"
TreeMenu "Timeslice`400`CurvatureAtt*" "Remove"
TreeMenu "Timeslice`400`DipAtt*" "Remove"

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_energyatt.jpg"
Ok
TreeButton "Timeslice`400`EnergyAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_eventatt.jpg"
Ok
TreeButton "Timeslice`400`EventAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_frequencytatt.jpg"
Ok
TreeButton "Timeslice`400`FrequencyAtt*" Off

#TreeMenu "Timeslice`400" "Add attribute"
#TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`FrequencyFilterAttrib"
#Button "Make snapshot"
#Input "Select filename" "ts_400_frequencyfilteratt.jpg"
#Ok
#TreeButton "Timeslice`400`FrequencyFilterAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`InstantaneousAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_instantaneoustatt.jpg"
Ok
TreeButton "Timeslice`400`InstantaneousAtt*" Off

TreeMenu "Timeslice`400`DipAngleAtt*" "Remove"
TreeMenu "Timeslice`400`EnergyAttr*" "Remove"
TreeMenu "Timeslice`400`EventAttr*" "Remove"
TreeMenu "Timeslice`400`FrequencyAttr*" "Remove"
#TreeMenu "Timeslice`400`FrequencyFil*" "Remove"

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_positionatt.jpg"
Ok
TreeButton "Timeslice`400`PositionAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_referenceshiftatt.jpg"
Ok
TreeButton "Timeslice`400`ReferenceShiftAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_scalingatt.jpg"
Ok
TreeButton "Timeslice`400`ScalingAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_similarityatt.jpg"
Ok
TreeButton "Timeslice`400`SimilarityAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_spectraldecompatt.jpg"
Ok
TreeButton "Timeslice`400`SpectralDecompAtt*" Off

TreeMenu "Timeslice`400`Instantaneous*" "Remove"
TreeMenu "Timeslice`400`PositionAttr*" "Remove"
TreeMenu "Timeslice`400`ReferenceShift*" "Remove"
TreeMenu "Timeslice`400`ScalingAttr*" "Remove"
TreeMenu "Timeslice`400`SimilarityAttr*" "Remove"

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`VelocityFanFilterAttrib"
Button "Make snapshot"
Input "Select filename" "ts_400_velocityfanfilteratt.jpg"
Ok
TreeButton "Timeslice`400`VelocityFanFilterAtt*" Off

TreeMenu "Timeslice`400" "Add attribute"
TreeMenu "Timeslice`400`<right-click>" "Sel*`Attributes`VolumeStatisticsAt*"
Button "Make snapshot"
Input "Select filename" "ts_400_volumestatisticsatt.jpg"
Ok
TreeButton "Timeslice`400`VolumeStatisticsAt*" Off

TreeMenu "Timeslice`400`SpectralDecomp*" "Remove"
TreeMenu "Timeslice`400`VelocityFan*" "Remove"

End

