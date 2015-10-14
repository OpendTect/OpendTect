dTect V3.2
OpendTect commands
Mon Jun 19 10:36:39 2008
!

Comment "------Generate Wavelet and Apply on Data-----------"
Case Insensitive

Menu "Survey`Manage`Wavelets"
Button "Generate"
Input "Central Frequency" 40
Input "Peak amplitude" 1
Button "Select Wavelet"
Input "Name" "2d-TestWavelet"
Ok
Ok
Button "Dismiss"

Comment "------------Adding ConvolveAttribute---------"
Menu "Analysis`Attributes"
Window "Attribute Set 2D"
Combo "Attribute group" "<All>"
Button "Save on OK" Off
Combo "Attribute Type" "Convolve"
Button "Select Input Data"
Button "Stored"
ListClick "Select Data" "LS 5k" Double
Combo "Filter type" "Wavelet"
Button "Select Wavelet"
ListClick "Objects list" "2d-TestWavelet" Double
Input "Attribute name" "Wavelet-on-2dData"
Ok
#Button "Add as new"

Comment "----------Apply Attibute On data----------"
Menu "Scenes`New"
Menu "Scenes`Tile`Horizontal"

TreeMenu "Tree Scene 1" "2d Seismics" "Add"
ListClick "Objects list" "LS 5k" Double
ListClick "Select Data from List" "i5007" Double
TreeMenu "Tree Scene 1" "2d Seismics`*`*`*" "Select*`Stored*`Seis"

TreeMenu "Tree Scene 2" "2d Seismics" "Add"
ListClick "Objects list" "LS 5k" Double
ListClick "Select Data from List" "i5007" Double
TreeMenu "Tree Scene 2" "2d Seismics`*`*`*" "Select*`Attributes`Wavelet*"
Wheel "hRotate" -45

Snapshot "$SNAPSHOTSDIR$/$IDX$_2d-ManageWavelet.png" ODMain

Sleep 2
Menu "Survey`Select/Setup"
Ok
Button "No"
Menu "Survey`Manage`Wavelets"
ListClick "Objects list" "2d-TestWavelet"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

End

