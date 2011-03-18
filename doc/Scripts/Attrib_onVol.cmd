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
Button "Cancel"
TreeMenu "Volume`*" "Select Attribute`Attributes`CoherencyAttrib"
TreeButton "Volume`CoherencyAttrib`Volren" On
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_coherencyatt_on_Vol.png"
Ok

TreeMenu "Volume`CoherencyAttr*" "Sel*`Att*`ConvolveAtt*"
Button "Make snapshot"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_convolveatt_on_Vol.png"
Ok


TreeMenu "Volume`ConvolveAtt*" "Sel*`Att*`CurvatureAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_curvatureatt_on_Vol.png"
Ok

#TreeMenu "Volume`CurvatureAttr*" "Sel*`Att*`DipAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_dipatt_on_Vol.png"
Ok

TreeMenu "Volume`DipAttr*" "Sel*`Att*`DipAngleAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_dipanglett_on_Vol.png"
Ok

TreeMenu "Volume`DipAngleAtt*" "Sel*`Att*`EnergyAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_energyatt_on_Vol.png"
Ok

TreeMenu "Volume`EnergyAttr*" "Sel*`Att*`EventAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_eventatt_on_Vol.png"
Ok

TreeMenu "Volume`EventAttr*" "Sel*`Att*`FrequencyAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_frequencyatt_on_Vol.png"
Ok

TreeMenu "Volume`FrequencyAtt*" "Sel*`Att*`FrequencyFilterAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_freqencyfilteratt_on_Vol.png"
Ok

TreeMenu "Volume`FrequencyFilterAtt*" "Sel*`Att*`GapdeconAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_gapdeconatt_on_Vol.png"
Ok

TreeMenu "Volume`GapdeconAtt*" "Sel*`Att*`Instant*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_instantaneousatt_on_Vol.png"
Ok

TreeMenu "Volume`Instant*" "Sel*`Att*`PositionAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_positionatt_on_Vol.png"
Ok

TreeMenu "Volume`PositionAtt*" "Sel*`Att*`ReferenceShi*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_refshiftatt_on_Vol.png"
Ok

TreeMenu "Volume`ReferenceSh*" "Sel*`Att*`ScalingAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_scalingatt_on_Vol.png"
Ok

TreeMenu "Volume`ScalingAtt*" "Sel*`Att*`SimilarityAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_similarityatt_on_Vol.png"
Ok

TreeMenu "Volume`SimilarityAtt*" "Sel*`Att*`SpectralDecomp*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_spectrapdecompatt_on_Vol.png"
Ok

TreeMenu "Volume`SpectralDecomp*" "Sel*`Att*`VelocityFan*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_velfanfilteratt_on_Vol.png"
Ok

TreeMenu "Volume`VelocityFan*" "Sel*`Att*`VolumeStatistics*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_volumestatt_on_Vol.png"
Ok

TreeMenu "Volume`VolumeStatistics*" "Remove"

Wheel "hRotate" -40
Wheel "vRotate" -10
Slider "Zoom Slider" 29

End
