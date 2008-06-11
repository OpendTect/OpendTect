dTect V3.2
OpendTect commands
Mon May 12 11:36:39 2008
!

Wheel "hRotate" 90
Slider "Zoom Slider" 10

#Applying Attrinutes to Crossline number 750
Comment "-------Applying Attrinutes to Crossline number 750-------"

TreeMenu "Crossline" "Add"
TreeMenu "Crossline`775" "Position"
Window "Positioning"
Input "Inl Start" 200
Input "Inl Stop" 450
Input "Crl nr" 750
Input "Z Start" 750
Input "Z Stop" 1400

Ok
ListClick "Select Data" "CoherencyAttrib"
Ok
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_coherencyatt.jpg"
Ok
TreeButton "Crossline`750`CoherencyAttr*" Off

OnError Continue
TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`ConvolveAttrib"
#Button "Make snapshot"
#Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_convolveatt.jpg"
#Ok
TreeButton "Crossline`750`ConvolveAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_curvatureatt.jpg"
Ok
TreeButton "Crossline`750`CurvatureAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_dipatt.jpg"
Ok
TreeButton "Crossline`750`DipAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_dipangleatt.jpg"
Ok
TreeButton "Crossline`750`DipAngleAttrib*" Off

TreeMenu "Crossline`750`CoherencyAttr*" "Remove"
TreeMenu "Crossline`750`ConvolveAttr*" "Remove"
TreeMenu "Crossline`750`CurvatureAtt*" "Remove"
TreeMenu "Crossline`750`DipAtt*" "Remove"

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_energyatt.jpg"
Ok
TreeButton "Crossline`750`EnergyAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_eventatt.jpg"
Ok
TreeButton "Crossline`750`EventAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_frequencyatt.jpg"
Ok
TreeButton "Crossline`750`FrequencyAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`FrequencyFilterAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_frequencyfilterattatt.jpg"
Ok
TreeButton "Crossline`750`FrequencyFilterAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`InstantaneousAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_instantaneousatt.jpg"
Ok
TreeButton "Crossline`750`InstantaneousAtt*" Off

TreeMenu "Crossline`750`DipAngleAtt*" "Remove"
TreeMenu "Crossline`750`EnergyAttr*" "Remove"
TreeMenu "Crossline`750`EventAttr*" "Remove"
TreeMenu "Crossline`750`FrequencyAttr*" "Remove"
TreeMenu "Crossline`750`FrequencyFil*" "Remove"


TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_positionatt.jpg"
Ok
TreeButton "Crossline`750`PositionAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_refshiftatt.jpg"
Ok
TreeButton "Crossline`750`ReferenceShiftAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_scalingatt.jpg"
Ok
TreeButton "Crossline`750`ScalingAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_similarityatt.jpg"
Ok
TreeButton "Crossline`750`SimilarityAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_spectraldecompatt.jpg"
Ok
TreeButton "Crossline`750`SpectralDecompAtt*" Off

TreeMenu "Crossline`750`Instantaneous*" "Remove"
TreeMenu "Crossline`750`PositionAttr*" "Remove"
TreeMenu "Crossline`750`ReferenceShift*" "Remove"
TreeMenu "Crossline`750`ScalingAttr*" "Remove"
TreeMenu "Crossline`750`SimilarityAttr*" "Remove"

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`VelocityFanFilterAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_velocityfanfilteratt.jpg"
Ok
TreeButton "Crossline`750`VelocityFanFilterAtt*" Off

TreeMenu "Crossline`750" "Add attribute"
TreeMenu "Crossline`750`<right-click>" "Sel*`Attributes`VolumeStatisticsAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_volumestatisticsatt.jpg"
Ok
TreeButton "Crossline`750`VolumeStatisticsA*" Off

TreeMenu "Crossline`750`SpectralDecomp*" "Remove"
TreeMenu "Crossline`750`VelocityFan*" "Remove"

End
