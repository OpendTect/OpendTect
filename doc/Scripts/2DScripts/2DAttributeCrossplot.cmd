dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

Comment "----------Script for Attribute Crossplot---------------"

Case Insensitive
Include "$SCRIPTSDIR$/2DScripts/AllAtributes.cmd"
Menu "Processing`Attributes"
Window "Attribute Set 2D"
Button "Cross-Plot attributes"
Window "Attribute cross-plotting"
ListSelect "Attributes to calculate" 1 4 On
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
Snapshot "$SNAPSHOTSDIR$/2d_coherencyattr_crossplot.png" CurWin
Window "[all] step*"
Close

Window "Attribute data"
TableClick "Data Table" ColHead "[Y]CoherencyAttrib"
Button "UnSelect as Y Data"
TableClick "Data Table" ColHead "ConvolveAtt*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/2d_convolveattr_crossplot.png" Curwin
Window "[all] step*"
Close

Window "Attribute data"
TableClick "Data Table" ColHead "[Y]ConvolveAttr*"
Button "UnSelect as Y Data"
TableClick "Data Table" ColHead "CurvatureAttr*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/2d_curvatureattr_crossplot.png" CurWin
Window "[all] step*"
Close

Window "Attribute data"
TableClick "Data Table" ColHead "[Y]Curvature*"
Button "UnSelect as Y Data"
TableClick "Data Table" ColHead "DipAttr*"
Button "Select as Y data"
Button "Show crossplot"
Snapshot "$SNAPSHOTSDIR$/2d_dipattr_crossplot.png"
Window "[all] step*"
Close

Window "Attribute data"
Button "Dismiss"

Window "Attribute cross-plotting"
Button "Cancel"

Window "Attribute Set 2D"
Ok

Menu "Survey`Select/Setup"
Ok
Button "No"

End
