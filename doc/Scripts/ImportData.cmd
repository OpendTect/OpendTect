dTect V4.2.0
OpendTect commands
May 2010
!
# $Id: ImportData.cmd,v 1.8 2011/03/18 16:23:44 cvsjaap Exp $

Case Insensitive

Comment "-------------Importing Horizon----------"
Menu "Survey`Import`Horizons`Ascii`Geometry 3D"
Input "Input ASCII File" "$EXPORTDIR$/HorizonExport.asc"

Button "Define Format definition" 
Window "Format definition"
Ok

Window "Import Horizon"
Button "Select Horizon" 
Input "Name" "Horizon-Import"
Ok
Button "Go"
[Information]
Ok
[Import Horizon]
Button "Dismiss"
Comment "-----------Displaying Above Imported Horizon-------"
[OpendTect Main Window]
TreeMenu "Horizon" "Load"
[Horizon selection]
ListClick "Objects list" "Horizon-Import" Double

Comment "----------------Importing Pickset------------"
[OpendTect Main Window]
Menu "Survey`Import`PickSets`Ascii"
[Import Pickset]
Input "Input Ascii file" "$EXPORTDIR$/PicksetExport.asc"
Button "Define Format definition" 
[Format definition]
Combo "Unit" "Seconds"
Ok
[Import Pickset]
Button "Select Output PickSet"
[Output PickSet]
Input "Name" "Pickset-Import"
Ok
[Import Pickset]
Button "Go"
[Information]
Button "Ok"
[Import Pickset]
Button "Dismiss"
Comment "--------Displaying Above Imported pickset--------"
[OpendTect Main Window]
TreeMenu "PickSet/Poly*" "Load PickSet"
[Load PickSet Group(s)]
ListClick "Objects list" "Pickset-Import" Double

Comment "----------------Importing Fault------------"
[OpendTect Main Window]
Menu "Survey`Import`Faults`Ascii 3D"
[Import Fault]
Input "Input ascii file" "$EXPORTDIR$/FaultExport.asc"
Button "Define Format defi*"
[Format definition]
Input "[Stick*" "col:4"
Ok
[Import Fault]
Button "Select Output Fault"
[Output Fault]
Input "Name" "TestFaultImport"
Ok
[Import Fault]
Button "Go"
[Information]
Button "Ok"
[Import Fault]
Button "Dismiss"

Comment "----------------Displaying above ImportedFault-----"
[OpendTect Main Window]
TreeMenu "Fault" "Load"
[Fault selection]
ListClick "Objects list" "TestFaultImport" Double

Comment "----------------Importing FaultStickSets--------"
[OpendTect Main Window]
Menu "Survey`Import`FaultStickSets`Ascii 3D"
[Import FaultStickSet] 
Input "Input ascii*" "$EXPORTDIR$/FaultSickSetExport.asc"
Button "Define Format*"
[Format definition]
Input "[Stick*" "col:4"
Ok
[Import FaultStickSet]
Button "Select Output FaultStickSet"
[Output FaultStickSet]
Input "Name" "TestFaultStickSetImport" 
Ok
[Import FaultStickSet]
Button "Go"
[Information]
Button "Ok"
[Import FaultStickSet]
Button "Dismiss"
Comment "------------Displaying above ImportedFaultStickSet--------"
[OpendTect Main Window]
TreeMenu "FaultStickSet" "Load"
[FaultStickSet selection]
ListClick "Objects list" "TestFaultStickSetImport" Double

Comment "----------Importing CBVS----------------"
[OpendTect Main Window]
Menu "Survey`Import`Seismics`CBVS"
[Import CBVS cube]
Input "Select (First) CBVS*" "$DATADIR$/Seismics/Median_Dip_Filtered_Seismics.cbvs"
Combo "Cube type" "Generated attribute*"
Button "Copy the data" On
Button "Select Volume subsel*"
[Positions]
Input "Inline start" 320
Input "Inline stop" 320
Ok
[Import CBVS cube]
Button "Select Output Cube"
[Save Median_Dip_Filtered_Seismics as]
Input "Name" "CBVS-Import"
Ok
[Import CBVS cube]
Button "Go"
[Information]
Button "Ok"

Comment "----------Importing Seismic Data--------------"

#Menu "Survey`Import`Seismics`SEG-Y"
#Window "SEG-Y tool"
#Input "Input SEG-Y*" "$EXPORTDIR$/SeisExport.sgy"
#Button "Next*"

#Window "Determine SEG-Y*"
#Ok
#Window "Import SEG-Y"
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
#OnError Continue
#Ok
#OnError Stop
#Window "SEG-Y Examiner"
#Button "Dismiss"
#Comment "---------Displaying Above imported Seismics---------"
#TreeMenu "Inline" "Add"
#TreeMenu "Inline`*" "Position"
#Window "Positioning"
#Input "Inl nr" 320
#Ok
#Button "Stored"
#ListClick "Select Data" "SeisImport" Double
#
#Wheel "hRotate" 45
#Wheel "vRotate" 45
#Button "Make snapshot"
#Button "Scene"
#Ok
#Input "Select filename" "$SNAPSHOTSDIR$/$FILEIDX$_TreeItems-imported"
#Ok
#Ok

#TreeMenu "Inline`*" "Remove"
TreeMenu "PickSet*`*" "Remove"
TreeMenu "Horizon`*" "Remove"
TreeMenu "Fault`*" "Remove"
TreeMenu "FaultStickSet`*" "Remove"

Wheel "vRotate" -45
Wheel "hRotate" -45

Button "Manage seismic data"
ListClick "Objects list" "SeisImport"
Button "Remove this object"
Button "Remove"
ListClick "Objects list" "CBVS-Import"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

Button "Manage horizons"
ListClick "Objects list" "Horizon-Import"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

Button "Manage PickSets*"
ListClick "Objects list" "Pickset-Import"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

Button "Manage Faults"
ListClick "Objects list" "TestFaultImport"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

ButtonMenu "Manage Faults" "FaultStickSets"
ListClick "Objects list" "TestFaultStickSetImport"
Button "Remove this object"
Button "Remove"
Button "Dismiss"

End
