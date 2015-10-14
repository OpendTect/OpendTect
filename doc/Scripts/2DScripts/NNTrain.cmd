dTect V3.2
OpendTect commands
Mon May 28 11:36:39 2008
!

Comment "--------NeuralNetwork Train----------"

Menu "Analysis`Neural Networks"
Button "Pattern recognition*"
Button "Unsupervised"
ListSelect "Select input attributes" "[LS 5k]" On
ListSelect "Select output nodes" 1 On
Ok
Ok
Sleep 8
Snapshot "$SNAPSHOTSDIR$/$IDX$_2d-NNTrain.png" CurWin
Ok
Button "Dismiss"

Comment "---------Displaying trained Data-----------"
Menu "Scenes`New"
Menu "Scenes`Tile`Horizontal"

TreeMenu "Tree scene 1" "2D Seismics" "Add"
ListClick "Objects list" "LS 5k" Double
ListClick "Select Data from List" "i5007" Double
TreeMenu "Tree scene 1" "2D Seismics`*`*`*" "Select Attribute`Stored 2D*`Seis"

TreeMenu "Tree scene 2" "2D Seismics" "Add"
ListClick "Objects list" "LS 5k" Double
ListClick "Select Data from List" "i5007" Double
TreeMenu "Tree scene 2" "2D Seismics`*`*`*" "Select Att*`Neural Net*`Segment"

Wheel "hRotate" -45
Slider "Zoom Slider" 20
Sleep 3
Snapshot "$SNAPSHOTSDIR$/$IDX$_2d-NNTrain-data.png" ODMain

Menu "Survey`Select/Setup"
Ok

End

