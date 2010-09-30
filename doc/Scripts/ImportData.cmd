dTect V4.2.0
OpendTect commands
Mon Jun 28 11:36:39 2008
!

Case Insensitive

Comment "-------------Importing Horizon----------"
Menu "Survey`Import`Horizons`Ascii`Geometry 3D"
Input "Input ASCII File" "$EXPORTDIR$/HorizonExport.asc"
Button "Define Format Def*" 
Ok
Button "Select Horizon" 
Input "Name" "Horizon-Import"
Ok
Button "Display after import" Off
Button "Go"
Button "Ok"
Button "Dismiss"
Comment "-----------Displaying Above Imported Horizon-------"
TreeMenu "Horizon" "Load"
ListClick "Objects list" "Horizon-Import" Double

Comment "----------------Importing Pickset------------"
Menu "Survey`Import`Picksets`Ascii"
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
TreeMenu "PickSet/Polygon" "Load PickSet"
ListClick "Objects list" "Pickset-Import" Double

Comment "----------------Importing Fault------------"
Menu "Survey`Import`Faults`Ascii 3D"
Input "Input ascii file" "$EXPORTDIR$/FaultExport.asc"
Button "Define Format defi*"
Input "[Stick*" "col:4"
Ok
Button "Select Output Fault"
Input "Name" "TestFaultImport"
Ok
Button "Go"
Ok
Button "Dismiss"

Comment "----------------Displaying above ImportedFault-----"
TreeMenu "Fault" "Load"
ListClick "Objects list" "TestFaultImport" Double

Comment "----------------Importing FaultStickSets--------"
Menu "Survey`Import`FaultStickSets`Ascii 3D"
Window "Import FaultStickSet"
Input "Input ascii*" "$EXPORTDIR$/FaultSickSetExport.asc"
Button "Define Format*"
Input "[Stick*" "col:4"
Ok
Button "Select Output FaultStickSet"
Input "Name" "TestFaultStickSetImport" 
Ok
Button "Go"
Ok
Button "Dismiss"
Comment "------------Displaying above ImportedFaultStickSet--------"
TreeMenu "FaultStickSet" "Load"
ListClick "Objects list" "TestFaultStickSetImport" Double

Comment "----------Importing CBVS----------------"
Menu "Survey`Import`Seismics`CBVS"
Input "Select (First) CBVS*" "$DATADIR$/Seismics/Median_Dip_Filtered_Seismics.cbvs"
Combo "Cube type" "Generated attribute*"
Button "Copy the data" On
Button "Select Volume subsel*"
Window "Positions"
Input "Inline start" 320
Input "Inline stop" 320
Ok
Button "Select Output Cube"
Input "Name" "CBVS-Import"
Ok
Ok
Ok

#Comment "----------Importing Seismic Data--------------"
#Menu "Survey`Import`Seismics`SEG-Y"
#Window "SEG-Y tool"
#Input "Input SEG-Y*" "$EXPORTDIR$/SeisExport.sgy"
#Button "Next>>"

#Window "Determine SEG-Y*"
#Ok
#Window "SEG-Y Scan"
#Button "Select Volume sub*"
#Window "Positions"
#Input "Inline start" 320
#Input "Inline stop" 320
#Ok
#Button "Select Output Cube"
#Input "Name" "SeisImport"
#Ok
#Ok
#Ok
#Comment "---------Displaying Above imported Seismics---------"
#TreeMenu "Inline" "Add"
#TreeMenu "Inline`*" "Position"
#Window "Positioning"
#Input "Inl nr" 320
#Ok
#Button "Stored"
#ListClick "Select Data" "Median Dip Filtered*" Double

Wheel "hRotate" 45
Wheel "vRotate" 45
Button "Make snapshot"
Button "Scene"
Ok
Input "Select filename" "$SNAPSHOTSDIR$/$IDX$_TreeItems-imported"
Ok

#TreeMenu "Inline`*" "Remove"
TreeMenu "PickSet/Polygon`*" "Remove"
TreeMenu "Horizon`*" "Remove"
TreeMenu "Fault`*" "Remove"
TreeMenu "FaultStickSet`*" "Remove"

Wheel "vRotate" -45
Wheel "hRotate" -45

Button "Manage Seismic data"
ListClick "Objects list" "CBVS-Import"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

Button "Manage horizons"
ListClick "Objects list" "Horizon-Import"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

Button "Manage PickSets/Polygons"
ListClick "Objects list" "Pickset-Import"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

ButtonMenu "Manage Faults" "Faults"
ListClick "Objects list" "TestFaultImport"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

ButtonMenu "Manage Faults" "Faultsticksets"
ListClick "Objects list" "TestFaultImport"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

End
