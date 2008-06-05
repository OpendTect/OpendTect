dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

TreeMenu "Well" "Load"
ListClick "Objects list" "F03-2" Double
TreeMenu "Well`F03-2" "Create attri*"
Button "Select Input*"
Button "Attributes"
ListClick "Select Data" "CoherencyAtt*"
Ok


Ok
Button "Yes"
OnError Continue
TreeMenu "Well`F03-2" "Select logs"
Combo "Select Left log" "CoherencyAttrib"
Button "Select Log Color"
ColorOk Red 2
Ok


End
