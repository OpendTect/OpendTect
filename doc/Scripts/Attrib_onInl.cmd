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

OnError Continue
ListClick "Select Data" "CoherencyAttrib"
Ok
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_coherencyatt.jpg"
Ok
TreeButton "Inline`320`CoherencyAttr*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`ConvolveAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_convolveatt.jpg"
Ok
TreeButton "Inline`320`ConvolveAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`CurvatureAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_curvatureatt.jpg"
Ok
TreeButton "Inline`320`CurvatureAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`DipAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_dipatt.jpg"
Ok
TreeButton "Inline`320`DipAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`DipAngleAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_dipangleatt.jpg"
Ok
TreeButton "Inline`320`DipAngleAtt*" Off

TreeMenu "Inline`320`CoherencyAttr*" "Remove"
TreeMenu "Inline`320`ConvolveAttr*" "Remove"
TreeMenu "Inline`320`CurvatureAtt*" "Remove"
TreeMenu "Inline`320`DipAtt*" "Remove"

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`EnergyAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_energyatt.jpg"
Ok
TreeButton "Inline`320`EnergyAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`EventAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_eventatt.jpg"
Ok
TreeButton "Inline`320`EventAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`FrequencyAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_frequencytatt.jpg"
Ok
TreeButton "Inline`320`FrequencyAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`FrequencyFil*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_frequencyfilteratt.jpg"
Ok
TreeButton "Inline`320`FrequencyFilterAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`InstantaneousAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_instantaneoustatt.jpg"
Ok
TreeButton "Inline`320`InstantaneousAtt*" Off

TreeMenu "Inline`320`DipAngleAtt*" "Remove"
TreeMenu "Inline`320`EnergyAttr*" "Remove"
TreeMenu "Inline`320`EventAttr*" "Remove"
TreeMenu "Inline`320`FrequencyAttr*" "Remove"
TreeMenu "Inline`320`FrequencyFil*" "Remove"

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`PositionAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_positionatt.jpg"
Ok
TreeButton "Inline`320`PositionAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`ReferenceShiftAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_referenceshiftatt.jpg"
Ok
TreeButton "Inline`320`ReferenceShiftAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`ScalingAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_scalingatt.jpg"
Ok
TreeButton "Inline`320`ScalingAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`SimilarityAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_similarityatt.jpg"
Ok
TreeButton "Inline`320`SimilarityAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`SpectralDecompAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_spectraldecompatt.jpg"
Ok
TreeButton "Inline`320`SpectralDecompAtt*" Off

TreeMenu "Inline`320`Instantaneous*" "Remove"
TreeMenu "Inline`320`PositionAttr*" "Remove"
TreeMenu "Inline`320`ReferenceShift*" "Remove"
TreeMenu "Inline`320`ScalingAttr*" "Remove"
TreeMenu "Inline`320`SimilarityAttr*" "Remove"

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`VelocityFanFilterAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_velocityfanfilteratt.jpg"
Ok
TreeButton "Inline`320`VelocityFanFilterAtt*" Off

TreeMenu "Inline`320" "Add attribute"
TreeMenu "Inline`320`<right-click>" "Sel*`Attributes`VolumeStatisticsAttrib"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/inl320_volumestatisticsatt.jpg"
Ok
TreeButton "Inline`320`VolumeStatisticsAtt*" Off

TreeMenu "Inline`320`SpectralDecomp*" "Remove"
TreeMenu "Inline`320`VelocityFan*" "Remove"

End
