dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Evaluate Energy Attribute-------"

TreeMenu "Inline" "Add"
TreeMenu "Inline`*" "Position"
Window "Positioning"
Input "Inl nr" 320
Ok
Button "Stored"
ListClick "Select Data" "Median Dip*" Double
TreeButton "Inline`320`Median Dip*" Off
TreeButton "Inline`320`Median Dip*" On
Include "$SCRIPTSDIR$/AttributeSet3DWindowPrepare.cmd"
Include "$SCRIPTSDIR$/energy.cmd"

Button "Evaluate attribute"
Window "Evaluate attribute"
Input "Initial value start" -10
Input "Initial value stop" 10
Input "Increment start" -5
Input "Increment stop" 5
Input "Nr of slices" 5
Button "Calculate"
Slider "Slice Slider" 0
Sleep 1
Window "OpendTect*"
Button "Make snapshot"
Button "Scene" 
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_evaluate_energy_10-10"
Ok
Sleep 1
Window "Evaluate attribute" 
Sleep 2
Slider "Slice Slider" 20
Window "OpendTect*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_evaluate_energy_15-15"
Ok
Sleep 1
Window "Evaluate attribute" 
Sleep 2
Slider "Slice Slider" 40 
Window "OpendTect*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_evaluate_energy_20-20"
Ok
Sleep 1
Window "Evaluate attribute" 
Sleep 2
Slider "Slice Slider" 80 
Window "OpendTect*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_evaluate_energy_25-25"
Ok
Sleep 1
Window "Evaluate attribute" 
Sleep 2
Slider "Slice Slider" 100 
Window "OpendTect*"
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_evaluate_energy_30-30"
Ok
Window "Evaluate attribute"
Close
Window "Attribute Set 3D"
Button "Redisplay element*"
#Window "Attribute Set 3D"
Button "Cancel"
TreeMenu "Inline`*" "Remove"
End
