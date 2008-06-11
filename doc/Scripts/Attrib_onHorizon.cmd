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
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_CoherencyAtt.png"

Wheel "vRotate" -85

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Convolve*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_ConvolveAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Curvature*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_CurvatureAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`dipatt*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_DipAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`dipangle*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_DipAngleAttt.png"

TreeMenu "Horizon`Demo 1*`CoherencyAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`ConvolveAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`CurvatureAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`DipAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`DipAngleAtt*" "Remove"

TreeMenu "Horizon`Demo 1*"  "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`energy*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_EnergyAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`event*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_EventAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`FrequencyAtt*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_FrequencyAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`FrequencyFil*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_FrequencyfilAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Instantaneous*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_InstantaneousAtt.png"

TreeMenu "Horizon`Demo 1*`EnergyAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`EventAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`FrequencyAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`FrequencyFil*" "Remove"
TreeMenu "Horizon`Demo 1*`InstantaneousAtt*" "Remove"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Position*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_PositionAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Referenceshifta*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_ReferenceshiftAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`scalingat*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_ScalingAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`SimilarityAtt*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_SimilarityAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`SpectralDec*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_SpectralDecAtt.png"

TreeMenu "Horizon`Demo 1*`PositionAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`ReferenceShiftAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`ScalingAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`SimilarityAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`SpectralDecompAtt*" "Remove"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`VelocityFanFi*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_VelocityFanFilAtt.png"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Volumestatistics*"
Snapshot "$DATADIR$/Misc/Snapshots/horDemo1_VolumestatisticsAtt.png"

Wheel "vRotate" -85

End
