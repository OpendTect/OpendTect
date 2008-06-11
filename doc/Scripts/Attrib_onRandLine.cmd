dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#Applying Attributes on tree item Random line.
Comment "----------Applying Attributes on tree item Random line---------"
Case Insensitive

OnError Continue
TreeMenu "Random line" "Remove all items"
Button "Yes"
OnError Stop

OnError Continue
TreeMenu "Random line`Random Line 1" "Remove"
OnError Stop

TreeMenu "Random line" "New"
TreeMenu "Random line`Random Line 1" "Edit nodes"
TableFill "BinID Table" 1 1 320
TableFill "BinID Table" 1 2 600
TableFill "BinID Table" 2 1 500
TableFill "BinID Table" 2 2 1000
Input "Z start" 950
Input "Z stop" 1700
Ok
ListClick "Select Data" "CoherencyAttrib"
Ok
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_400_coherencyeatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`CoherencyAtt*" "Sel*`Att*`ConvolveAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl__convolveatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`ConvolveAtt*" "Sel*`Att*`CurvatureAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl__curvatureatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`CurvatureAtt*" "Sel*`Att*`DipAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_dipatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`DipAtt*" "Sel*`Att*`DipAngleAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_dipangleatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`DipAngleAtt*" "Sel*`Att*`EnergyAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_energyatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`EnergyAtt*" "Sel*`Att*`EventAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_eventatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`EventAtt*" "Sel*`Att*`FrequencyAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_frequencyatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`FrequencyAtt*" "Sel*`Att*`FrequencyFil*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_frequencyfliteratt.jpg"
Ok

TreeMenu "Random line`Random Line 1`FrequencyAtt*" "Sel*`Att*`Instan*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_instantaneousatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`Instanta*" "Sel*`Att*`PositionAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_positionatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`PositionAtt*" "Sel*`Att*`ReferenceSh*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_refshiftatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`ReferenceSh*" "Sel*`Att*`ScalingAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_scalingatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`Scaling*" "Sel*`Att*`SimilarityAtt*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_similarityatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`SimilarityAtt*" "Sel*`Att*`SpectralDe*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_spectraldecompatt.jpg"
Ok

TreeMenu "Random line`Random Line 1`SpectralDe*" "Sel*`Att*`VelocityFan*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_velfanfilteratt.jpg"
Ok

TreeMenu "Random line`Random Line 1`VelocityFan*" "Sel*`Att*`VolumeStat*"
Button "Make snapshot"
Input "Select filename" "$SCRIPTSDIR$/Snapshots/rl_volstatatt.jpg"
Ok

End
