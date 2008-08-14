dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Script for Attribute Crossplot---------------"

Case Insensitive
Menu "processing`Attributes"
Window "Attribute Set 2D"
Button "New attribute set"
OnError Continue
Button "No"
OnError Stop
Combo "Attribute group" "<All>"
Button "Save on OK" off
Combo "Attribute type" "Coherency"
Button "Select*"
ListClick "Select Data" 1 Double
Input "Attribute name" "CoherencyAttrib"

Button "Cross-Plot attributes"
Window "Attribute cross-plotting"
ListClick "Attributes to calculate" "CoherencyAttrib"
Combo "Line name" "i5007"
Button "Select Location filters"
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
Snapshot "$SCRIPTSDIR$/Snapshots/2Dattribcrossplot.png" CurWin

Window "Attribute data"
Button "Dismiss"

Window "Attribute cross-plotting"
Button "Cancel"

Window "Attribute Set 2D"
Ok

End
