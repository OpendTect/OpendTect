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
Input "Inline stop" 300
Input "Crossline start" 850
Input "Crossline stop"  1050
Ok
Ok

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Coherency*"
Combo "Table selection" "Brown 4grades"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_CoherencyAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Coherency*" "Select*`Att*`Convolve*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_ConvolveAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Convolve*" "Select*`Att*`Curvature*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_CurvatureAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Curvature*" "Select*`Att*`DipAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_DipAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`DipAtt*" "Select*`Att*`DipAngle*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_DipangleAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`DipAngle*" "Select*`Att*`Energy*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_EnergyAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Energy*" "Select*`Att*`Event*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_EventAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Event*" "Select*`Att*`FrequencyAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_FrequencyAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`FrequencyAtt*" "Select*`Att*`FrequencyFil*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_FreqFilterAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`FrequencyFil*" "Select*`Att*`Instantaneous*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_InstantaAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Instantaneous*" "Select*`Att*`Position*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_PositionAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`Position*" "Select*`Att*`ReferenceShiftA*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_ReferShiftAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`ReferenceShiftA*" "Select*`Att*`ScalingAt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_ScalingAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`ScalingAt*" "Select*`Att*`SimilarityAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_SimilarityAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`SimilarityAtt*" "Select*`Att*`SpectralDec*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_SpectralDecAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`SpectralDec*" "Select*`Att*`VelocityFanFi*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_VelFanfilAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*`VelocityFanFi*" "Select*`Att*`Volumestatistics*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SCRIPTSDIR$/Snapshots/horDemo1_VolStatiAtt.jpg"
Ok

TreeMenu "Horizon`Demo 1*" "Remove"

Slider "Zoom Slider" 29 
Wheel "vRotate" -85

End
