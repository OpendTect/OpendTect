dTect V3.2
OpendTect commands
Mon Jun 19 10:36:39 2008
!

Case Insensitive
Comment "---------------Importing Well------------"
Menu "Survey`Manage`Wells"
Window "Well file*"
ListClick "Objects list" "F03-2"
ListClick "Available logs" "Gamma Ray"
Button "Export log"
Window "Export Well*"
Input "Depth range (m) start" 400
Input "Depth range (m) stop" 600
Input "Output file" "$EXPORTDIR$/GammaRay.dat"
Ok
Comment "---------------Importing above log------"
Button "Add Logs"
Window "Logs"
Input "Input*" "$EXPORTDIR$/GammaRay.dat"
Ok
Window "Well file*"
Button "Dismiss"
Comment "---------Displaying above iported log--------"
TreeMenu "Well" "Load"
ListClick "Objects list" "F03-2" Double
TreeMenu "Well`*" "Properties"
Window "Well display*"
Tab "Well display*" "Left Log"
Combo "Select log" "Gamma_Ray"
Button "Log filled" Off
Button "Line color"
ColorOK Red 4
Button "Dismiss"
Snapshot "$SNAPSHOTSDIR$/$FILEIDX$_WellImport.png" CurWin
TreeMenu "Well`*" "Remove"
Menu "Survey`Manage`Wells"
ListClick "Objects list" "F03-2"
ListClick "Available logs" "Gamma_Ray"
Button "Rename selected*"
Input "New name" "Gamma_Ray_new"
ListClick "Available logs" "Gamma_Ray_new"
Button "Remove selected*"
Button "Yes"
Button "Dismiss"

End
