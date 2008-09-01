dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Script for Attribute Crossplot---------------"

Case Insensitive
Menu "processing`Attributes"
Window "Attribute Set 3D"
Button "New attribute set"
OnError Continue
Button "No"
OnError Stop
Combo "Attribute group" "<All>"
Button "Save on OK" off
Include "$SCRIPTSDIR$/coherency.cmd"
Include "$SCRIPTSDIR$/convolve.cmd"
Include "$SCRIPTSDIR$/energy.cmd"
Include "$SCRIPTSDIR$/frequency.cmd"

Button "Cross-Plot attributes"
Window "Attribute cross-plotting"
ListSelect "Attributes to*" 1 4 On
Combo "Select locations by" "Range"
Input "Inline start" 300
Input "Inline stop" 350
Input "Crossline start" 500
Input "Crossline stop" 1200
Input "Z start" 0
Input "Z stop" 1800
Button "Select Location*"
ListButton "Filter selection" "Subsample" On
Input "Pass one*" 4
Ok
Ok

Window "Attribute data"
TableClick "Data Table" ColHead "X-Coord"
Button "Set data for X"
TableClick "Data Table" ColHead "CoherencyAttrib"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/CoherencyAttrCrossplot.png" CurWin
Sleep 5
Window "300/500*"
Close

Window "Attribute data"
Button "UnSelect as Y data" 
TableClick "Data Table" ColHead "ConvolveAtt*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/ConvAttrCrossplot.png"
Sleep 5
Window "300/500*"
Close

Window "Attribute data"
Button "UnSelect as Y data"
TableClick "Data Table" ColHead "EnergyAttr*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/EnergyAttrCrossplot.png"
Sleep 5
Window "300/500*"
Close

Window "Attribute data"
Button "UnSelect as Y data"
TableClick "Data Table" ColHead "FrequencyAttr*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/FreqAttrCrossplot.png"
Sleep 5
Window "300/500*"
Close

Window "Attribute data"
Button "Dismiss"

Window "Attribute cross-plotting"
Button "Cancel"

Window "Attribute Set 3D"
Ok

End
