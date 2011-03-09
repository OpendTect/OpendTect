dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Script for Attribute Crossplot---------------"

Include "$SCRIPTSDIR$/AttributeSet3DWindowPrepare.cmd"
Include "$SCRIPTSDIR$/convolve.cmd"
Include "$SCRIPTSDIR$/energy.cmd"
Include "$SCRIPTSDIR$/frequency.cmd"
Ok
Menu "Analysis`Attributes"
Window "Attribute Set 3D"

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
TableClick "Data Table" ColHead "*CoherencyAttrib"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/$IDX$_CoherencyAttrCrossplot.png" CurWin
Sleep 2
Window "300/500*"
Close

Window "Attribute data"
Button "UnSelect as Y data" 
TableClick "Data Table" ColHead "ConvolveAtt*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/$IDX$_ConvAttrCrossplot.png"
Sleep 2
Window "300/500*"
Close

Window "Attribute data"
Button "UnSelect as Y data"
TableClick "Data Table" ColHead "EnergyAttr*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/$IDX$_EnergyAttrCrossplot.png"
Sleep 2
Window "300/500*"
Close

Window "Attribute data"
Button "UnSelect as Y data"
TableClick "Data Table" ColHead "FrequencyAttr*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/$IDX$_FreqAttrCrossplot.png"
Sleep 2
Window "300/500*"
Close

Window "Attribute data"
Button "Dismiss"

#--------CrossPlot using PickSet------

Window "Attribute cross-plotting"
Combo "Select locations*" "Table"
Button "Pick Set"
Button "Select Pickset Group"
ListClick "Objects list" 1 Double
Ok
Window "Attribute data"
TableClick "Data Table" ColHead "Z*"
Button "Set data for X"
TableClick "Data Table" ColHead "*CoherencyAtt*"
Button "Select as Y data"
Button "Show crossplot"
Sleep 2
#-------------------------------
Window "1000 points*"
Close
Window "Attribute data"
Button "Dismiss"

Window "Attribute cross-plotting"
Button "Cancel"

Window "Attribute Set 3D"
Ok

Menu "Survey`Select/Setup"
Ok
Button "No"

End
