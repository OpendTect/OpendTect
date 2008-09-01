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
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/coherencyatt_on_Vol.jpg"
Ok

TreeMenu "Volume`CoherencyAttr*" "Sel*`Att*`ConvolveAtt*"
Button "Make snapshot"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/convolveatt_on_Vol.jpg"
Ok


TreeMenu "Volume`ConvolveAtt*" "Sel*`Att*`CurvatureAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/curvatureatt_on_Vol.jpg"
Ok

TreeMenu "Volume`CurvatureAttr*" "Sel*`Att*`DipAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/dipatt_on_Vol.jpg"
Ok

TreeMenu "Volume`DipAttr*" "Sel*`Att*`DipAngleAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/dipanglett_on_Vol.jpg"
Ok

TreeMenu "Volume`DipAngleAtt*" "Sel*`Att*`EnergyAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/energyatt_on_Vol.jpg"
Ok

TreeMenu "Volume`EnergyAttr*" "Sel*`Att*`EventAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/eventatt_on_Vol.jpg"
Ok

TreeMenu "Volume`EventAttr*" "Sel*`Att*`FrequencyAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/frequencyatt_on_Vol.jpg"
Ok

TreeMenu "Volume`FrequencyAtt*" "Sel*`Att*`FrequencyFilterAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/freqencyfilteratt_on_Vol.jpg"
Ok

TreeMenu "Volume`FrequencyFilterAtt*" "Sel*`Att*`Instant*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/instantaneousatt_on_Vol.jpg"
Ok

TreeMenu "Volume`Instant*" "Sel*`Att*`PositionAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/positionatt_on_Vol.jpg"
Ok

TreeMenu "Volume`PositionAtt*" "Sel*`Att*`ReferenceShi*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/refshiftatt_on_Vol.jpg"
Ok

TreeMenu "Volume`ReferenceSh*" "Sel*`Att*`ScalingAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/scalingatt_on_Vol.jpg"
Ok

TreeMenu "Volume`ScalingAtt*" "Sel*`Att*`SimilarityAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/similarityatt_on_Vol.jpg"
Ok

TreeMenu "Volume`SimilarityAtt*" "Sel*`Att*`SpectralDecomp*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/spectrapdecompatt_on_Vol.jpg"
Ok

TreeMenu "Volume`SpectralDecomp*" "Sel*`Att*`VelocityFan*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/velfanfilteratt_on_Vol.jpg"
Ok

TreeMenu "Volume`VelocityFan*" "Sel*`Att*`VolumeStatistics*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/volumestatt_on_Vol.jpg"
Ok

TreeMenu "Volume`VolumeStatistics*" "Remove"

Wheel "hRotate" -40
Wheel "vRotate" -10
Slider "Zoom Slider" 29

End
