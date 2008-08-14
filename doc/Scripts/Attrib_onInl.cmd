dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#Applying Attrinutes to Inline number 320
Comment "-----Applying Attrinutes to Inline number 320------"
TreeMenu "Inline" "Add"
TreeMenu "Inline`425" "Position"
Window "Positioning"
Input "Inl nr" 320
Input "Crl Start" 600
Input "Crl Stop" 1000
Input "Z Start" 900
Input "Z Stop" 1500
Ok

ListClick "Select Data" "CoherencyAttrib"
Ok
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_coherencyatt.jpg"
Ok

TreeMenu "Inline`320`CoherencyAttr*" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_convolveatt.jpg"
Ok

TreeMenu "Inline`320`ConvolveAttr*" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_curvatureatt.jpg"
Ok

TreeMenu "Inline`320`CurvatureAttrib" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_dipatt.jpg"
Ok

TreeMenu "Inline`320`DipAttrib" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_dipangleatt.jpg"
Ok

TreeMenu "Inline`320`DipAngleAttrib" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_energyatt.jpg"
Ok

TreeMenu "Inline`320`EnergyAttrib" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_eventatt.jpg"
Ok

TreeMenu "Inline`320`EventAttrib" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_frequencytatt.jpg"
Ok

TreeMenu "Inline`320`FrequencyAttrib" "Sel*`Attributes`FrequencyFilterAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_frequencyfilteratt.jpg"
Ok

TreeMenu "Inline`320`FrequencyFilterAttrib" "Sel*`Attributes`InstantaneousA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_instantaneoustatt.jpg"
Ok

TreeMenu "Inline`320`InstantaneousAttrib" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_positionatt.jpg"
Ok

TreeMenu "Inline`320`PositionAttrib" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_referenceshiftatt.jpg"
Ok

TreeMenu "Inline`320`ReferenceShiftAttrib" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_scalingatt.jpg"
Ok

TreeMenu "Inline`320`ScalingAttrib" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_similarityatt.jpg"
Ok

TreeMenu "Inline`320`SimilarityAttrib" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_spectraldecompatt.jpg"
Ok


TreeMenu "Inline`320`SpectralDecomp*" "Sel*`Attributes`VelocityFanFilter*"
Button "Make snapshot"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_velocityfanfilteratt.jpg"
Ok

TreeMenu "Inline`320`VelocityFanFilter*" "Sel*`Attributes`VolumeStatisticsAt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_volumestatisticsatt.jpg"
Ok

TreeMenu "Inline`320" "Remove"

End
