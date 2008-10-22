dTect V3.2
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Case Insensitive
Comment "----------Importing Seismic Data--------------"

Menu "Survey`Import`Seismics`SEG-Y"
Window "SEG-Y tool"
Input "Input SEG-Y*" "$EXPORTDIR$/SeisExport.sgy"
Ok
Window "Determine SEG-Y*"
Ok

Window "SEG-Y Scan"
Button "Select Volume sub*"
Input "Inline start" 320
Input "Inline stop" 320
Ok
Button "Select Output Cube"
Input "Name" "SeisImport"
Ok
Ok
Ok
#Window "SEG-Y Examiner""
#Button "Dismiss"
Comment "---------Displaying Above imported Seismics---------"
TreeMenu "Inline" "Add"
TreeMenu "Inline`*" "Position"
Window "Positioning"
Input "Inl nr" 320
Ok
Button "Stored"
ListClick "Select Data" "Median Dip Filtered*" Double


Comment "-------------Importing Horizon----------"
Menu "Survey`Import`Horizons`Ascii`Geometry 3D"
Input "Input ASCII File" "$EXPORTDIR$/HorizonExport.asc"
Button "Define Format Def*" 
Ok
Button "Select Horizon" 
Input "Name" "Horizon-Import"
Ok
Button "Go"
Button "Dismiss"

Comment "-----------Displaying Above Imported Horizon-------"
TreeMenu "Horizon" "Load"
ListClick "Select Horizon*" "Horizon-Import" Double


Comment "----------------Importing Pickset------------"
Menu "Survey`Import`Picksets"
Input "Input Ascii file" "$EXPORTDIR$/PicksetExport.asc"
Button "Define Format definition" 
Combo "Unit" "Seconds"
Ok
Button "Select Output PickSet"
Input "Name" "Pickset-Import"
Ok
Button "Go"
Ok
Button "Dismiss"
Comment "--------Displaying Above Imported pickset--------"
TreeMenu "Pickset" "Load"
ListClick "Objects list" "Pickset-Import" Double


Comment "---------------Importing Well------------"
Menu "Survey`Import`Wells`Ascii`Track"
Input "Well Track File" "$DATADIR$/WellInfo/Track/F02-1.track"
Button "Define Format definition"
Input "[MD]" "col:4"
Ok
Input "Depth to Time model*" "$DATADIR$/WellInfo/DT_model/F02-01_TD.txt"
Input "Elevation above*" 10
Button "Select Output Well"
Input "Name" "F02-1-Import"
Ok
Button "Go"
Ok
Button "Dismiss"
Comment "---------------Displaying above Imported Well---------------"
TreeMenu "Well" "Load"
ListClick "Objects list" "F02-1-Import" Double
TreeMenu "Well`*" "Properties"
Button "Color"
ColorOK Blue 4
Ok


Comment "----------------Importing Fault------------"
#Menu "Survey`Import`Faults`Ascii 3D"
#Input "Input ascii file" "$EXPORTDIR$/Export/FaultExport.asc"
#Button "Define Format defi*"
#Ok
#Button "Select Output Fault"
#Input "Name" "TestFaultImport.flt"
#Ok
#Button "Dismiss"

Wheel "hRotate" 45
Wheel "vRotate" 45
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/TreeItems-imported"
Ok

TreeMenu "Inline`*" "Remove"
TreeMenu "PickSet`*" "Remove"
TreeMenu "Horizon`*" "Remove"
Treemenu "Well`*" "Remove"
Wheel "vRotate" -45
Wheel "hRotate" -45

Button "Manage seismic data"
ListClick "Objects list" "SeisImport"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

Button "Manage horizons"
ListClick "Objects list" "Horizon-Import"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

Button "Manage well data"
ListClick "Objects list" "F02-1-Import"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

Button "Manage Pick Sets"
ListClick "Objects list" "Pickset-Import"
Button "Remove this object"
Button "Yes"
Button "Dismiss"

End
