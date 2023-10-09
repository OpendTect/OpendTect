#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include "stringview.h"

/*!
\brief is used for defining key strings that are 'global'.

The idea is also that we get some uniformity in how we read/write things
from/to file. Thus, if you suspect a string is rather common, try to find
something similar here first.

Also, some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

namespace sKey
{
    inline StringView All()		{ return "All"; }
    inline StringView Ascii()		{ return "Ascii"; }
    inline StringView Alignment()	{ return "Alignment"; }
    inline StringView Attribute()	{ return "Attribute"; }
    inline StringView Attribute2D()	{ return "Attribute2D"; }
    inline StringView Attributes()	{ return "Attributes"; }
    inline StringView Average()		{ return "Average"; }
    inline StringView Azimuth()		{ return "Azimuth"; }
    inline StringView Binary()		{ return "Binary"; }
    inline StringView Body()		{ return "Body"; }
    inline StringView Category()	{ return "Category"; }
    inline StringView Chain()		{ return "Chain"; }
    inline StringView Class()		{ return "Class"; }
    inline StringView Classes()		{ return "Classes"; }
    inline StringView ClassNm()		{ return "Class Name"; }
    inline StringView Color()		{ return "Color"; }
    inline StringView Component()	{ return "Component"; }
    inline StringView Components()	{ return "Components"; }
    inline StringView Content()		{ return "Content"; }
    inline StringView CoordSys()	{ return "Coordinate System"; }
    inline StringView CrlRange()	{ return "Cross-line range"; }
    inline StringView Crossline()	{ return "Cross-line"; }
    inline StringView Cube()		{ return "Cube"; }
    inline StringView Data()		{ return "Data"; }
    inline StringView DataRoot()	{ return "Data Root"; }
    inline StringView DataSet()		{ return "DataSet"; }
    inline StringView DataStorage()	{ return "Data storage"; }
    inline StringView DataType()	{ return "DataType"; }
    inline StringView Date()		{ return "Date"; }
    inline StringView DateTime()	{ return "Date/Time"; }
    inline StringView Default()		{ return "Default"; }
    inline StringView DefaultDataRoot()	{ return "Default Data Root"; }
    inline StringView Depth()		{ return "Depth"; }
    inline StringView DepthRange()	{ return "Depth Range"; }
    inline StringView Desc()		{ return "Description"; }
    inline StringView Distribution()	{ return "Distribution"; }
    inline StringView EmptyString()	{ return ""; }
    inline StringView Err()		{ return "Err"; }
    inline StringView Examples()	{ return "Examples"; }
    inline StringView Factor()		{ return "Factor"; }
    inline StringView Fault()		{ return "Fault"; }
    inline StringView FaultLikelihood()	{ return "Fault Likelihood"; }
    inline StringView FaultStick()	{ return "FaultStick"; }
    inline StringView FileName()	{ return "File name"; }
    inline StringView Filter()		{ return "Filter"; }
    inline StringView FirstCrl()	{ return "First Cross-line"; }
    inline StringView FirstInl()	{ return "First In-line"; }
    inline StringView FirstTrc()	{ return "First Trace"; }
    inline StringView FirstZ()		{ return "First Z"; }
    inline StringView FloatUdf()	{ return "1e30"; }
    inline StringView Font()		{ return "Font"; }
    inline StringView Format(int n=1)	{ return n<2 ? "Format" : "Formats"; }
    inline StringView GeomID()		{ return "GeomID"; }
    inline StringView GeomIDs()		{ return "GeomIDs"; }
    inline StringView GeomSystem()	{ return "GeomSystem"; }
    inline StringView Geometry()	{ return "Geometry"; }
    inline StringView Histogram()	{ return "Histogram"; }
    inline StringView Horizon()		{ return "Horizon"; }
    inline StringView Horizon2D()	{ return "Horizon2D"; }
    inline StringView Hostname()	{ return "Hostname"; }
    inline StringView ID()		{ return "ID"; }
    inline StringView IDs()		{ return "IDs"; }
    inline StringView IOSelection()	{ return "I/O Selection"; }
    inline StringView InlRange()	{ return "In-line range"; }
    inline StringView Inline()		{ return "In-line"; }
    inline StringView Input()		{ return "Input"; }
    inline StringView IPAddress()	{ return "IP Address"; }
    inline StringView Is2D()		{ return "Is2D"; }
    inline StringView IsLink()		{ return "Is Link"; }
    inline StringView Keys()		{ return "Keys"; }
    inline StringView LastCrl()		{ return "Last Cross-line"; }
    inline StringView LastInl()		{ return "Last In-line"; }
    inline StringView LastTrc()		{ return "Last Trace"; }
    inline StringView LastZ()		{ return "Last Z"; }
    inline StringView Level()		{ return "Level"; }
    inline StringView Line()		{ return "Line"; }
    inline StringView LineKey()		{ return "Line key"; }
    inline StringView LineName()	{ return "Line name"; }
    inline StringView LineNames()	{ return "Line names"; }
    inline StringView LineStyle()	{ return "Line Style"; }
    inline StringView Lines()		{ return "Lines"; }
    inline StringView Local()		{ return "Local"; }
    inline StringView Log()		{ return "Log"; }
    inline StringView LogFile()		{ return "Log file"; }
    inline StringView Logs()		{ return "Logs"; }
    inline StringView MD(int n=1)	{ return n<2 ? "MD" : "MDs"; }
    inline StringView Marker()		{ return "Marker"; }
    inline StringView Markers()		{ return "Markers"; }
    inline StringView MarkerStyle()	{ return "Marker Style"; }
    inline StringView Maximum()		{ return "Maximum"; }
    inline StringView Median()		{ return "Median"; }
    inline StringView Minimum()		{ return "Minimum"; }
    inline StringView Mnemonics()	{ return "Mnemonics"; }
    inline StringView Mode()		{ return "Mode"; }
    inline StringView Model()		{ return "Model"; }
    inline StringView Models()		{ return "Models"; }
    inline StringView Name()		{ return "Name"; }
    inline StringView Names()		{ return "Names"; }
    inline StringView NewLine()		{ return "\n"; }
    inline StringView No()		{ return "No"; }
    inline StringView None()		{ return "None"; }
    inline StringView NrFaults()	{ return "Nr Faults";}
    inline StringView NrGeoms()		{ return "Nr Geometries";}
    inline StringView NrItems()		{ return "Nr Items";}
    inline StringView NrPicks()		{ return "Nr Picks"; }
    inline StringView NrValues()	{ return "Nr Values"; }
    inline StringView Object()		{ return "Object"; }
    inline StringView Offset()		{ return "Offset"; }
    inline StringView OffsetRange()	{ return "Offset Range"; }
    inline StringView Ok()		{ return "Ok"; }
    inline StringView Order()		{ return "Order"; }
    inline StringView Output()		{ return "Output"; }
    inline StringView OutputID()	{ return "output_id"; }
    inline StringView Pars()		{ return "Parameters"; }
    inline StringView PickSet()		{ return "PickSet"; }
    inline StringView Polygon()		{ return "Polygon"; }
    inline StringView PointSet()	{ return "PointSet"; }
    inline StringView Position()	{ return "Position"; }
    inline StringView PreStackSeis()	{ return "PreStack Seismic"; }
    inline StringView ProjSystem()	{ return "ProjectionBased System"; }
    inline StringView Projection()	{ return "Projection"; }
    inline StringView Property()	{ return "Property"; }
    inline StringView Python()		{ return "Python"; }
    inline StringView Quiet()		{ return "quiet"; }
    inline StringView RMS()		{ return "RMS"; }
    inline StringView Random()		{ return "Random"; }
    inline StringView RandomLine()	{ return "RandomLine"; }
    inline StringView Range()		{ return "Range"; }
    inline StringView Region()		{ return "Region"; }
    inline StringView Sampling()	{ return "Sampling"; }
    inline StringView Scale()		{ return "Scale"; }
    inline StringView Scaling()		{ return "Scaling"; }
    inline StringView SeisCubePositions() { return "Seismic Cube Positions"; }
    inline StringView SeisID()		{ return "SeisID"; }
    inline StringView Selection()	{ return "Selection"; }
    inline StringView SelectionStatus()	{ return "Selection status"; }
    inline StringView Series()		{ return "Series"; }
    inline StringView Server()		{ return "Server"; }
    inline StringView ServerNm()	{ return "Server_Name"; }
    inline StringView Setup()		{ return "Setup"; }
    inline StringView Shape()		{ return "Shape"; }
    inline StringView Shortcuts()	{ return "Shortcuts"; }
    inline StringView Shotpoint()	{ return "Shotpoint"; }
    inline StringView Size()		{ return "Size"; }
    inline StringView SliceType()	{ return "SliceType"; }
    inline StringView Source()		{ return "Source"; }
    inline StringView SpaceString()	{ return " "; }
    inline StringView StackOrder()	{ return "Stack order"; }
    inline StringView Stats()		{ return "Stats"; }
    inline StringView Status()		{ return "Status"; }
    inline StringView StdDev()		{ return "StdDev"; }
    inline StringView Steering()	{ return "Steering"; }
    inline StringView StepCrl()		{ return "Step Cross-line"; }
    inline StringView StepInl()		{ return "Step In-line"; }
    inline StringView StepOut()		{ return "Stepout"; }
    inline StringView StepOutCrl()	{ return "Stepout Cross-line"; }
    inline StringView StepOutInl()	{ return "Stepout In-line"; }
    inline StringView StepTrc()		{ return "Step Trace"; }
    inline StringView StepZ()		{ return "Step Z"; }
    inline StringView Stored()		{ return "Stored"; }
    inline StringView StratRef()	{ return "Strat Level"; }
    inline StringView Subsample()	{ return "Subsample"; }
    inline StringView Subsel()		{ return "Subsel"; }
    inline StringView Sum()		{ return "Sum"; }
    inline StringView Surface()		{ return "Surface"; }
    inline StringView Survey()		{ return "Survey"; }
    inline StringView SurveyID()	{ return "Survey ID"; }
    inline StringView TVD(int n=1)	{ return n<2 ? "TVD" : "TVDs"; }
    inline StringView TVDSS()		{ return "TVDSS"; }
    inline StringView TWT()		{ return "TWT"; }
    inline StringView Table()		{ return "Table"; }
    inline StringView Target()		{ return "Target"; }
    inline StringView Targets()		{ return "Targets"; }
    inline StringView TermEm()		{ return "Terminal Emulator"; }
    inline StringView Thickness()	{ return "Thickness"; }
    inline StringView ThreeD()		{ return "3D"; }
    inline StringView Time()		{ return "Time"; }
    inline StringView TimeRange()	{ return "Time Range"; }
    inline StringView Title()		{ return "Title"; }
    inline StringView TmpStor()		{ return "Temporary storage location"; }
    inline StringView TraceKey()	{ return "Trace key"; }
    inline StringView TraceNr()		{ return "Trace number"; }
    inline StringView Transparency()	{ return "Transparency"; }
    inline StringView TrcRange()	{ return "Trace Range"; }
    inline StringView TwoD()		{ return "2D"; }
    inline StringView Type()		{ return "Type"; }
    inline StringView Types()		{ return "Types"; }
    inline StringView Undef()		{ return "Undefined"; }
    inline StringView Unit()		{ return "Unit"; }
    inline StringView Units()		{ return "Units"; }
    inline StringView Unscale()		{ return "Unscale"; }
    inline StringView User()		{ return "User"; }
    inline StringView Value()		{ return "Value"; }
    inline StringView ValueRange()	{ return "Value Range"; }
    inline StringView Values()		{ return "Values"; }
    inline StringView Variance()	{ return "Variance"; }
    inline StringView Velocity()	{ return "Velocity"; }
    inline StringView Version()		{ return "Version"; }
    inline StringView Volume()		{ return "Volume"; }
    inline StringView WaveletID()	{ return "Wavelet ID"; }
    inline StringView Weight()		{ return "Weight"; }
    inline StringView Well()		{ return "Well"; }
    inline StringView X()		{ return "X"; }
    inline StringView XCoord()		{ return "X-Coord"; }
    inline StringView XCoords()		{ return "X-Coords"; }
    inline StringView XOffset()		{ return "X Offset"; }
    inline StringView XRange()		{ return "X range"; }
    inline StringView Y()		{ return "Y"; }
    inline StringView Y2()		{ return "Y2"; }
    inline StringView YCoord()		{ return "Y-Coord"; }
    inline StringView YCoords()		{ return "Y-Coords"; }
    inline StringView YOffset()		{ return "Y Offset"; }
    inline StringView YRange()		{ return "Y range"; }
    inline StringView Yes()		{ return "Yes"; }
    inline StringView Z()		{ return "Z"; }
    inline StringView ZCoord()		{ return "Z-Coord"; }
    inline StringView ZDomain()		{ return "Z Domain"; }
    inline StringView ZRange()		{ return "Z range"; }
    inline StringView ZSlice()		{ return "Z-slice"; }
    inline StringView ZStep()		{ return "Z step"; }
    inline StringView ZUnit()		{ return "Z-Unit"; }
    inline StringView ZValue()		{ return "Z value"; }

    inline StringView DepthUnit()	{ return "Depth-Unit"; }
    //<!To be used only when two different Z unit are stored
    inline StringView TimeUnit()	{ return "Time-Unit"; }
    //<!To be used only when two different Z unit are stored

    // History of objects
    inline StringView CrBy()		{ return "Created.By"; }
    inline StringView CrAt()		{ return "Created.At"; }
    inline StringView CrFrom()		{ return "Created.From"; }
    inline StringView CrInfo()		{ return "Created.Info"; }
    inline StringView ModBy()		{ return "Modified.By"; }
    inline StringView ModAt()		{ return "Modified.At"; }

    // Not used?
    inline StringView CrFtPolygonDir() { return "FaultPolygonPath"; }
    inline StringView StrFtPolygon()   { return "FaultPolygons"; }
};
