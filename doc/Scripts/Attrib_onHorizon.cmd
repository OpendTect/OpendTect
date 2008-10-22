dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Case Insensitive

Comment "--------Applying Attributes On Tree item Horizon-------"
Wheel "vRotate" 85
Slider "Zoom Slider" 22

TreeMenu "Horizon" "Load"
ListClick "Sel*" "Demo 1*" 
Button "Select*" 
Combo "Area sub*" "Range"
Input "Inline start" 110
Input "Inline stop" 150
Input "Crossline start" 850
Input "Crossline stop"  1000
Ok
Ok

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Coherency*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/CoherencyAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Coherency*" "Select*`Att*`Convolve*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/ConvolveAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Convolve*" "Select*`Att*`Curvature*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/CurvatureAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Curvature*" "Select*`Att*`DipAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/DipAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`DipAtt*" "Select*`Att*`DipAngle*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/DipangleAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`DipAngle*" "Select*`Att*`Energy*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/EnergyAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Energy*" "Select*`Att*`Event*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/EventAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Event*" "Select*`Att*`FrequencyAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/FrequencyAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`FrequencyAtt*" "Select*`Att*`FrequencyFil*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/FreqFilterAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`FrequencyFil*" "Select*`Att*`Instantaneous*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/InstantaAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Instantaneous*" "Select*`Att*`Position*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/PositionAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Position*" "Select*`Att*`ReferenceShiftA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/ReferShiftAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`ReferenceShiftA*" "Select*`Att*`ScalingAt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/ScalingAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`ScalingAt*" "Select*`Att*`SimilarityAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/SimilarityAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`SimilarityAtt*" "Select*`Att*`SpectralDec*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/SpectralDecAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`SpectralDec*" "Select*`Att*`VelocityFanFi*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/VelFanfilAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*`VelocityFanFi*" "Select*`Att*`Volumestatistics*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/VolStatiAtti_on_horDemo1.jpg"
Ok

TreeMenu "Horizon`Demo 1*" "Remove"

Slider "Zoom Slider" 29 
Wheel "vRotate" -85

End
