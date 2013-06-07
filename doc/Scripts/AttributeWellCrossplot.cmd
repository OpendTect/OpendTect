dTect V4.2.0
OpendTect commands
!

# $Id: AttributeWellCrossplot.cmd,v 1.6 2010/10/13 09:06:08 cvsnageswara Exp $

[OpendTect Main Window]
Menu "Analysis`Attributes"

[Attribute Set 3D]
Combo "Attribute group" "<All>"
Button "Save on OK" Off
Combo "Attribute type" "Convolve"
Button "Select Input Data"

[Select]
ListClick "Select Data" "Median Dip Filtered Seismics" Left
Button "Ok"

[Attribute Set 3D]
Input "Attribute name" "ConvolveAttrib" Hold
Button "Add as new"
Combo "Attribute type" "Energy"
Button "Select Input Data"

[Select]
Button "Stored" On
ListClick "Select Data" "Median Dip Filtered Seismics" Left
Button "Ok"

[Attribute Set 3D]
Input "Attribute name" "EnergyAttrib" Hold
Button "Add as new"
Button "Ok"

[OpendTect Main Window]
Menu "Analysis`Cross-plot`Well logs <--> Attributes"

[Attribute/Well cross-plotting]
ListSelect "Attributes" "ConvolveAttrib" "EnergyAttrib"
ListClick "Wells" "F02-1" Left
ListClick "Logs" "Caliper" Left
Button "Select Filter positions"

[Filters]
ListButton "Filter selection" "Subsample" On
Button "Ok"

[Attribute/Well cross-plotting]
Button "Ok"

[Well attribute data]
TableClick "Data Table" ColHead "X-Coord" 
Button "Set data for X"
TableClick "Data Table" ColHead "Caliper" 
Button "Select as Y data"
Button "Show crossplot"

[Well data / Attributes Cross-plot]
Close

[Well attribute data]
TableClick "Data Table" ColHead "ConvolveAttrib" 
TableClick "Data Table" ColHead "[Y]Caliper" 
Button "UnSelect as Y data"
Button "Set Y one column right"
Button "Set Y one column right"
Button "Set Y one column right"
Button "Set Y one column right"
Button "Set Y one column right"
Button "Set Y one column right"
Button "Show crossplot"

[Well data / Attributes Cross-plot]
Close

[Well attribute data]
TableClick "Data Table" ColHead "[Y]ConvolveAttrib" 
Button "UnSelect as Y data"
TableClick "Data Table" ColHead "EnergyAttrib" 
Button "Show histogram and stats for column"

[Data statistics]
Close

[Well attribute data]
Button "Select as Y data"
Button "Show crossplot"

[Well data / Attributes Cross-plot]
Close

[Well attribute data]
Button "Dismiss"

[Attribute/Well cross-plotting]
Button "Cancel"

[OpendTect Main Window]
Menu "Survey`Select/Setup"

[Survey selection]
Button "Edit"

[Survey setup]
Button "Ok"

[Survey selection]
Button "Ok (Select)"

[Data not saved]
Button "Don't save"

[OpendTect Main Window]
