dTect V3.2
OpendTect commands
Mon Jun 19 10:36:39 2008
!

Case Insensitive

Comment "------Generate Wavelet and Apply on Data-----------"

Menu "Survey`Manage`Wavelets"
#Window "Wavelet management"
Button "Generate"
Input "Central Frequency" 40
Input "Peak amplitude" 1
Button "Select Wavelet"
Input "Name" "TestWavelet"
Ok
Ok
Button "Dismiss"

Comment "------------Adding ConvolveAttribute---------"

Menu "Analysis`Attributes"
Window "Attribute Set 3D"
Combo "Attribute group" "<All>"
Combo "Attribute Type" "Convolve"
Button "Select Input Data"
Button "Stored"
ListClick "Select Data" "Median Dip Filter*" Double
Combo "Filter type" "Wavelet"
Button "Select Wavelet"
ListClick "Objects list" "TestWavelet" Double
Input "Attribute name" "Wavelet-on-MedianDip"
Button "Save on OK" Off
Ok

Comment "----------Apply Attibute On data----------"

Menu "Scenes`New"
Menu "Scenes`Tile`Horizontal"

TreeMenu "Tree Scene 1" "Inline" "Add"
TreeMenu "Tree Scene 1" "Inline`*`*" "Select Attribute`Stored Cubes`Median Dip Filter*"

TreeMenu "Tree Scene 2" "Inline" "Add"
TreeMenu "Tree Scene 2" "Inline`*`*" "Select Attribute`Attributes`Wavelet-on-MedianDip"

Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_ManageWavelet.png" ODMain

Sleep 2
#Window "OpendTect Main Window"
Menu "Survey`Select/Setup"
Ok
Button "No"
Menu "Survey`Manage`Wavelets"
ListClick "Objects list" "TestWavelet"
Button "Rename this object"
Input "New name" "New-TestWavelet"
ListClick "Objects list" "New-TestWavelet"
Button "Remove this Object"
Button "Yes"
Button "Dismiss"

End
