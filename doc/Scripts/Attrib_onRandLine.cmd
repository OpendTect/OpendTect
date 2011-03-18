dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

#Applying Attributes on tree item Random line.
Comment "----------Applying Attributes on tree item Random line---------"
Case Insensitive

TreeMenu "Random line" "New"
TreeMenu "Random line`Random Line 1" "Edit nodes"
TableFill "BinID Table" 1 1 320
TableFill "BinID Table" 1 2 600
TableFill "BinID Table" 2 1 500
TableFill "BinID Table" 2 2 1000
Input "Z start" 950
Input "Z stop" 1700
Ok
ListClick "Select Data" "CoherencyAttrib" Double
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_coherencyeatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`CoherencyAtt*" "Sel*`Att*`ConvolveAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_convolveatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`ConvolveAtt*" "Sel*`Att*`CurvatureAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_curvatureatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`CurvatureAtt*" "Sel*`Att*`DipAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_dipatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`DipAtt*" "Sel*`Att*`DipAngleAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_dipangleatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`DipAngleAtt*" "Sel*`Att*`EnergyAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_energyatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`EnergyAtt*" "Sel*`Att*`EventAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_eventatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`EventAtt*" "Sel*`Att*`FrequencyAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_frequencyatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`FrequencyAtt*" "Sel*`Att*`FrequencyFil*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_frequencyfliteratt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`FrequencyFil*" "Sel*`Att*`GapdeconAt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_gapdeconatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`GapdeconAtt*" "Sel*`Att*`Instan*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_instantaneousatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`Instanta*" "Sel*`Att*`PositionAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_positionatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`PositionAtt*" "Sel*`Att*`ReferenceSh*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_refshiftatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`ReferenceSh*" "Sel*`Att*`ScalingAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_scalingatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`Scaling*" "Sel*`Att*`SimilarityAtt*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_similarityatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`SimilarityAtt*" "Sel*`Att*`SpectralDe*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_spectraldecompatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`SpectralDe*" "Sel*`Att*`VelocityFan*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_velfanfilteratt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1`VelocityFan*" "Sel*`Att*`VolumeStat*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_volstatatt_on_rl.jpg"
Ok

TreeMenu "Random line`Random Line 1" "Remove"

End
