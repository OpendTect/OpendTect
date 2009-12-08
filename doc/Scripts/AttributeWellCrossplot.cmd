dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Script for Attribute/Well Crossplot---------------"

Include "$SCRIPTSDIR$/AttributeSet3DWindowPrepare.cmd"
Include "$SCRIPTSDIR$/coherency.cmd"
Include "$SCRIPTSDIR$/convolve.cmd"
Include "$SCRIPTSDIR$/energy.cmd"
Include "$SCRIPTSDIR$/frequency.cmd"
Ok

Menu "Analysis`Cross-plot`Well logs*"
Window "Attribute/Well cross-plotting"
ListSelect "Attributes" 1 4 On
ListSelect "Wells" 1 On
ListSelect "Logs" "Sonic" On
Input "Radius around wells" 25
Button "Select Filter position*"
ListButton "Filter selection" 2 Off
ListButton "Filter selection" 2 On
Input "Pass one every" 2
Ok
Ok

Window "Well attribute data"
TableClick "Data Table" ColHead "*DAH"
Button "Set data for X"
TableClick "Data Table" ColHead "CoherencyAtt*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/$IDX$_CoherencyAttrWellCrossplot.png" 
Sleep 3
Window "Well data*"
Close

Window "Well attribute data"
TableClick "Data Table" ColHead "[Y]CoherencyAttr*" 
Button "UnSelect as Y data"
TableClick "Data Table" ColHead "ConvolveAttr*"
Button "Select as Y Data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/$IDX$_ConvolAttrWellCrossplot.png"
Window "Well data*"
Close
Window "Well attribute data"
TableClick "Data Table" ColHead "[Y]ConvolveAtt*"
Button "UnSelect as Y data"

TableClick "Data Table" ColHead "EnergyAtt*"
Button "Select as Y Data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/$IDX$_EnergyAttrWellCrossPlot.png"
Window "Well data*"
Close
Window "Well attribute data"
TableClick "Data Table" ColHead "[Y]EnergyAtt*"
Button "UnSelect as Y data"

TableClick "Data Table" ColHead "FrequencyAttr*"
Button "Select as Y Data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/$IDX$_FrequencyAttrWellCrossplot.png"
Window "Well data*"
Close

Window "Well attribute data"
Button "Dismiss"

Window "Attribute/Well cross-plotting"
Button "Cancel"

Menu "Survey`Select/Setup"
Ok
Button "No"

End
