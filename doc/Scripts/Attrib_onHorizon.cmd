dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "--------Loading Horizon-------"
Wheel "vRotate" 75

TreeMenu "Horizon" "Load"
Button "Select" 
Combo "Area sub*" "Range"
Spin "Inline start" 196
Spin "Inline stop" -348
Spin "Inline step" 0
Spin "Crossline start" 8
Spin "Crossline stop"  -648
Spin "Crossline step" 0
Button "Ok"

ListClick "Sel*" "Demo 1*" 
Ok

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Coherency*"

iTreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Convolve*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Curvature*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`dipatt*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`dipangle*"

TreeMenu "Horizon`Demo 1*`CoherencyAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`ConvolveAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`CurvatureAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`DipAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`DipAngleAtt*" "Remove"

TreeMenu "Horizon`Demo 1*"  "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`energy*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`event*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`FrequencyAtt*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`FrequencyFil*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Instantaneous*"

TreeMenu "Horizon`Demo 1*`EnergyAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`EventAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`FrequencyAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`FrequencyFil*" "Remove"
TreeMenu "Horizon`Demo 1*`InstantaneousAtt*" "Remove"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Position*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`referenceshifta*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`scalingat*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`similarityatt*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`SpectralDec*"

TreeMenu "Horizon`Demo 1*`PositionAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`ReferenceShiftAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`ScalingAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`SimilirityAtt*" "Remove"
TreeMenu "Horizon`Demo 1*`SpectralDecompAtt*" "Remove"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`VelocityFanFi*"

TreeMenu "Horizon`Demo 1*" "Add attribute"
TreeMenu "Horizon`Demo 1*`<right-click>" "Select*`Att*`Volumestatistics*"

End
