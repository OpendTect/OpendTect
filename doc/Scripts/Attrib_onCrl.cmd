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
Input "Inl Stop" 400
Input "Crl nr" 750
Input "Z Start" 750
Input "Z Stop" 1200
Ok
Button "Cancel"
TreeMenu "Crossline`*`*" "Select Attribute`Attributes`CoherencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_coherencyatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`CoherencyAttr*" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/convolveatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`ConvolveAttr*" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_curvatureatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`CurvatureAttrib*" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_dipatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`DipAttrib" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_dipangleatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`DipAngleAttrib" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_energyatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`EnergyAttrib" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_eventatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`EventAttrib" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_frequencytatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`FrequencyAttrib" "Sel*`Attributes`FrequencyFilterAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_frequencyfilteratt_on_crl750.png"
Ok

TreeMenu "Crossline`750`FrequencyFilterAttrib" "Sel*`Attributes`GapdeconAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_gapdeconatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`GapdeconAttrib" "Sel*`Attributes`InstantaneousA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_instantaneoustatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`InstantaneousAttrib" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_positionatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`PositionAttrib" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_referenceshiftatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`ReferenceShiftAttrib" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_scalingatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`ScalingAttrib" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_similarityatt_on_crl750.png"
Ok

TreeMenu "Crossline`750`SimilarityAttrib" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_spectraldecompatt_on_crl750.png"
Ok


TreeMenu "Crossline`750`SpectralDecomp*" "Sel*`Attributes`VelocityFanFilter*"
Button "Make snapshot"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_velocityfanfilteratt_on_crl750.png"
Ok

TreeMenu "Crossline`750`VelocityFanFilter*" "Sel*`Attributes`VolumeStatistics*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_volumestatisticsatt_on_crl750.png"
Ok

TreeMenu "Crossline`750" "Remove"

Wheel "hRotate" -90
Slider "Zoom Slider" 29

End
