dTect V3.2
OpendTect commands
Mon Jan 28 11:36:39 2008
!

TreeMenu "Well" "Load"
ListClick "Objects list" "F03-2" Double
TreeMenu "Well`F03-2" "Create attri*"
Button "Select Input*"
Button "Attributes"
ListClick "Select Data" "CoherencyAttr*" Double
Input "Log name" "CoherencyAttr_onF03-2"
Ok

TreeMenu "Well`F03-2" "Properties"
Window "Well display*"
Tab "Well display*" "Markers"
Input "Size" 10
Tab "Well display*" "Left Log"
Combo "Select log" "CoherencyAttr_onF03-2"
Input "Size" 2
Button "Line color"
ColorOk Red 2
Tab "Well display*" "Right Log"
Combo "Select log" 3
Input "Size" 2
Button "Line color"
ColorOk Blue 2
Button "log filled" Off
Button "Dismiss"
Menu "Survey`Manage`Wells"
ListClick "Objects list" "F03-2"
ListClick "Available logs" "CoherencyAttr_onF03-2"
Button "Remove selected log"
Button "Remove"
Button "Dismiss"
TreeMenu "Well`*" "Remove"

End

