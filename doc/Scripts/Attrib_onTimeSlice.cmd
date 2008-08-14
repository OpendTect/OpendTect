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
Input "Inl Stop" 350
Input "Crl Start" 850
Input "Crl Stop" 1150
Input "Z" 400
Ok
ListClick "Select Data" "CoherencyAttrib" Double

Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_coherencyatt.jpg"
Ok

TreeMenu "Timeslice`400`CoherencyAttr*" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_convolveatt.jpg"
Ok

TreeMenu "Timeslice`400`ConvolveAttr*" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_curvatureatt.jpg"
Ok

TreeMenu "Timeslice`400`CurvatureAttrib" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_dipatt.jpg"
Ok

TreeMenu "Timeslice`400`DipAttrib" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_dipangleatt.jpg"
Ok

TreeMenu "Timeslice`400`DipAngleAttrib" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_energyatt.jpg"
Ok

TreeMenu "Timeslice`400`EnergyAttrib" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_eventatt.jpg"
Ok

TreeMenu "Timeslice`400`EventAttrib" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_frequencytatt.jpg"
Ok

TreeMenu "Timeslice`400`FrequencyAttrib" "Sel*`Attributes`FrequencyFilterAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_frequencyfilteratt.jpg"
Ok

TreeMenu "Timeslice`400`FrequencyFilterAttrib" "Sel*`Attributes`InstantaneousA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_instantaneoustatt.jpg"
Ok

TreeMenu "Timeslice`400`InstantaneousAttrib" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_positionatt.jpg"
Ok

TreeMenu "Timeslice`400`PositionAttrib" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_referenceshiftatt.jpg"
Ok

TreeMenu "Timeslice`400`ReferenceShiftAttrib" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_scalingatt.jpg"
Ok

TreeMenu "Timeslice`400`ScalingAttrib" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_similarityatt.jpg"
Ok

TreeMenu "Timeslice`400`SimilarityAttrib" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_spectraldecompatt.jpg"
Ok


TreeMenu "Timeslice`400`SpectralDecomp*" "Sel*`Attributes`VelocityFanFilter*"
Button "Make snapshot"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_velocityfanfilteratt.jpg"
Ok

TreeMenu "Timeslice`400`VelocityFanFilter*" "Sel*`Attributes`VolumeStatisticsAt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/ts_400_volumestatisticsatt.jpg"
Ok

TreeMenu "Timeslice`400" "Remove"

Wheel "vRotate" -90
Slider "Zoom Slider" 29

End

