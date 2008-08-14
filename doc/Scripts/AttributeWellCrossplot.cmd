dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Script for Attribute/Well Crossplot---------------"

Case Insensitive
Menu "processing`Cross-plot"
Window "Attribute/Well cross-plotting"
ListClick "Attributes" "[Median Dip Filtered Seismics]" 
ListSelect "Wells" 1 On
ListSelect "Logs" "Sonic" On
Input "Radius around wells" 25
Button "Select Filter position*"
ListButton "Filter selection" 2 Off
ListButton "Filter selection" 2 On
Input "Pass one every" 2
Ok
Combo "Top marker" "MFS11"
Combo "Bottom marker" "FS6"
Ok

Window "Well attribute data"
TableClick "Data Table" ColHead "DAH"
Button "Set data for X"
TableClick "Data Table" ColHead "[Median dip*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SCRIPTSDIR$/Snapshots/attrwellcrossplot.png" CurWin
Sleep 5

Window "Well attribute data"
Button "Dismiss"

Window "Attribute/Well cross-plotting"
Button "Cancel"

End
