dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!


#Assining Values to Timeslice number 400
Comment "--------Assining Values to Timeslice number 400-----------"
Wheel "vRotate" 90

TreeMenu "Timeslice" "Add"
TreeMenu "Timeslice`924" "Position"
window "Positioning"
Input "Inl Start" 160
Input "Inl Stop" 200
Input "Crl Start" 850
Input "Crl Stop" 950
Input "Z" 400
Ok
Button "Cancel"
TreeMenu "Timeslice`*`*" "Select Attribute`Attributes`CoherencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_coherencyatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`CoherencyAttr*" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_convolveatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`ConvolveAttr*" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_curvatureatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`CurvatureAttrib" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_dipatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`DipAttrib" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_dipangleatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`DipAngleAttrib" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_energyatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`EnergyAttrib" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_eventatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`EventAttrib" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_frequencytatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`FrequencyAttrib" "Sel*`Attributes`FrequencyFilterAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_frequencyfilteratt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`FrequencyFilterAttrib" "Sel*`Attributes`GapdeconAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_gapdeconatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`GapdeconAttrib" "Sel*`Attributes`InstantaneousA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_instantaneoustatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`InstantaneousAttrib" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_positionatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`PositionAttrib" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_referenceshiftatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`ReferenceShiftAttrib" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_scalingatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`ScalingAttrib" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_similarityatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`SimilarityAttrib" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_spectraldecompatt_on_ts400.png"
Ok


TreeMenu "Timeslice`400`SpectralDecomp*" "Sel*`Attributes`VelocityFanFilter*"
Button "Make snapshot"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_velocityfanfilteratt_on_ts400.png"
Ok

TreeMenu "Timeslice`400`VelocityFanFilter*" "Sel*`Attributes`VolumeStatisticsAt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_volumestatisticsatt_on_ts400.png"
Ok

TreeMenu "Timeslice`400" "Remove"

Wheel "vRotate" -90
Slider "Zoom Slider" 29

End

