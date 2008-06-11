dTect V3.2
OpendTect commands
Mon May 8 11:36:39 2008
!

Wheel "vRotate" 10
Wheel "hRotate" 40
Slider "Zoom Slider" 8
#Assigning Values to Volume

Comment "----------Assigning Values to Tree item Volume----------"

TreeMenu "Volume" "Add"
TreeMenu "Volume`<right-click>" "Position"
Input "Inl Start" 400
Input "Inl Stop" 405
Input "Crl Start" 700
Input "Crl Stop" 710
Input "Z Start" 710
Input "Z Stop" 730
Ok
ListClick "Select Data" "CoherencyAttrib"
Ok
TreeButton "Volume`CoherencyAttrib`Volren" On
Button "Make snapshot"
Input "Select filename" "vol_coherencyatt.jpg"
Ok

TreeMenu "Volume`CoherencyAttr*" "Sel*`Att*`ConvolveAtt*"
Button "Make snapshot"
Input "Select filename" "vol_convolveatt.jpg"
Ok


TreeMenu "Volume`ConvolveAtt*" "Sel*`Att*`CurvatureAtt*"
Button "Make snapshot"
Input "Select filename" "vol_curvatureatt.jpg"
Ok

TreeMenu "Volume`CurvatureAttr*" "Sel*`Att*`DipAtt*"
Button "Make snapshot"
Input "Select filename" "vol_dipatt.jpg"
Ok

TreeMenu "Volume`DipAttr*" "Sel*`Att*`DipAngleAtt*"
Button "Make snapshot"
Input "Select filename" "vol_dipanglett.jpg"
Ok

TreeMenu "Volume`DipAngleAtt*" "Sel*`Att*`EnergyAtt*"
Button "Make snapshot"
Input "Select filename" "vol_energyatt.jpg"
Ok

TreeMenu "Volume`EnergyAttr*" "Sel*`Att*`EventAtt*"
Button "Make snapshot"
Input "Select filename" "vol_eventatt.jpg"
Ok

TreeMenu "Volume`EventAttr*" "Sel*`Att*`FrequencyAtt*"
Button "Make snapshot"
Input "Select filename" "vol_frequencyatt.jpg"
Ok

#TreeMenu "Volume`FrequencyAtt*" "Sel*`Att*`FrequencyFilterAtt*"
#Button "Make snapshot"
#Input "Select filename" "vol_freqencyfilteratt.jpg"
#Ok

TreeMenu "Volume`FrequencyAtt*" "Sel*`Att*`Instant*"
Button "Make snapshot"
Input "Select filename" "vol_instantaneousatt.jpg"
Ok

TreeMenu "Volume`Instant*" "Sel*`Att*`PositionAtt*"
Button "Make snapshot"
Input "Select filename" "vol_positionatt.jpg"
Ok

TreeMenu "Volume`PositionAtt*" "Sel*`Att*`ReferenceShi*"
Button "Make snapshot"
Input "Select filename" "vol_refshiftatt.jpg"
Ok

TreeMenu "Volume`ReferenceSh*" "Sel*`Att*`ScalingAtt*"
Button "Make snapshot"
Input "Select filename" "vol_scalingatt.jpg"
Ok

TreeMenu "Volume`ScalingAtt*" "Sel*`Att*`SimilarityAtt*"
Button "Make snapshot"
Input "Select filename" "vol_similarityatt.jpg"
Ok

TreeMenu "Volume`SimilarityAtt*" "Sel*`Att*`SpectralDecomp*"
Button "Make snapshot"
Input "Select filename" "vol_spectrapdecompatt.jpg"
Ok

TreeMenu "Volume`SpectralDecomp*" "Sel*`Att*`VelocityFan*"
Button "Make snapshot"
Input "Select filename" "vol_velfanfilteratt.jpg"
Ok

TreeMenu "Volume`VelocityFan*" "Sel*`Att*`VolumeStatistics*"
Button "Make snapshot"
Input "Select filename" "vol_volumestatt.jpg"
Ok

Wheel "vRotate" -10
Wheel "hRotate" -40

End
