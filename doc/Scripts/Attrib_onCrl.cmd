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
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl_750_coherencyatt.jpg"
Ok

TreeMenu "Crossline`750`CoherencyAttr*" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_convolveatt.jpg"
Ok

TreeMenu "Crossline`750`ConvolveAttr*" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_curvatureatt.jpg"
Ok

TreeMenu "Crossline`750`CurvatureAttrib" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_dipatt.jpg"
Ok

TreeMenu "Crossline`750`DipAttrib" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_dipangleatt.jpg"
Ok

TreeMenu "Crossline`750`DipAngleAttrib" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_energyatt.jpg"
Ok

TreeMenu "Crossline`750`EnergyAttrib" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_eventatt.jpg"
Ok

TreeMenu "Crossline`750`EventAttrib" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_frequencytatt.jpg"
Ok

TreeMenu "Crossline`750`FrequencyAttrib" "Sel*`Attributes`FrequencyFilterAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_frequencyfilteratt.jpg"
Ok

TreeMenu "Crossline`750`FrequencyFilterAttrib" "Sel*`Attributes`InstantaneousA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_instantaneoustatt.jpg"
Ok

TreeMenu "Crossline`750`InstantaneousAttrib" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_positionatt.jpg"
Ok

TreeMenu "Crossline`750`PositionAttrib" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_referenceshiftatt.jpg"
Ok

TreeMenu "Crossline`750`ReferenceShiftAttrib" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_scalingatt.jpg"
Ok

TreeMenu "Crossline`750`ScalingAttrib" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_similarityatt.jpg"
Ok

TreeMenu "Crossline`750`SimilarityAttrib" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_spectraldecompatt.jpg"
Ok


TreeMenu "Crossline`750`SpectralDecomp*" "Sel*`Attributes`VelocityFanFilter*"
Button "Make snapshot"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_velocityfanfilteratt.jpg"
Ok

TreeMenu "Crossline`750`VelocityFanFilter*" "Sel*`Attributes`VolumeStatistics*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/crl750_volumestatisticsatt.jpg"
Ok

TreeMenu "Crossline`750" "Remove"

Wheel "hRotate" -90
Slider "Zoom Slider" 29

End
