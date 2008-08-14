dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#Applying Attrinutes to Line i5007 
Comment "-----Applying Attrinutes to Line i5007------"

TreeMenu "2D Seismics" "Add"
ListClick "Objects list" 1 double
ListClick "Select Data from List" 1 Double
TreeMenu "2D Seismics`*`*`*" "Select Attribute`Attributes`CoherencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_coherencyatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_convolveatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_curvatureatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_dipatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_dipangleatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_energyatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_eventatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_frequencytatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`FrequencyFilterAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_frequencyfilteratt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`InstantaneousA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_instantaneoustatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_positionatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_referenceshiftatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_scalingatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_similarityatt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_spectraldecompatt.jpg"
Ok


TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`VelocityFanFilter*"
Button "Make snapshot"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_velocityfanfilteratt.jpg"
Ok

TreeMenu "2D Seismics`*`*`*" "Sel*`Attributes`VolumeStatisticsAt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/2d_volumestatisticsatt.jpg"
Ok

TreeMenu "2D Seismics`*" "Remove"
Button "Yes"

End
