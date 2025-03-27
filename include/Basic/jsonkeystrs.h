#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2025 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"
#include "stringview.h"

/*!
\brief is used for defining key strings that are used in JSON objects.
They need to follow the camelCase style.

The idea is also that we get some uniformity in how we read/write things
from/to file. Thus, if you suspect a string is rather common, try to find
something similar here first.

Also, some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

namespace sJSONKey
{
    inline StringView Alignment()	{ return "alignment"; }
    inline StringView Attribute()	{ return "attribute"; }
    inline StringView Attribute2D()	{ return "attribute2D"; }
    inline StringView Attributes()	{ return "attributes"; }
    inline StringView Azimuth()		{ return "azimuth"; }
    inline StringView Category()	{ return "category"; }
    inline StringView Chain()		{ return "chain"; }
    inline StringView Class()		{ return "class"; }
    inline StringView Classes()		{ return "classes"; }
    inline StringView ClassNm()		{ return "className"; }
    inline StringView Color()		{ return "color"; }
    inline StringView Component()	{ return "component"; }
    inline StringView Components()	{ return "components"; }
    inline StringView Content()		{ return "content"; }
    inline StringView CoordSys()	{ return "coordinateSystem"; }
    inline StringView Coordinate()	{ return "coordinate"; }
    inline StringView CrlRange()	{ return "crosslineRange"; }
    inline StringView Crossline()	{ return "crossline"; }
    inline StringView Data()		{ return "data"; }
    inline StringView DataRoot()	{ return "dataRoot"; }
    inline StringView DataSet()		{ return "dataSet"; }
    inline StringView DataStorage()	{ return "dataStorage"; }
    inline StringView DataType()	{ return "dataType"; }
    inline StringView Date()		{ return "date"; }
    inline StringView Default()		{ return "default"; }
    inline StringView DefaultDataRoot() { return "defaultDataRoot"; }
    inline StringView Depth()		{ return "depth"; }
    inline StringView DepthRange()	{ return "depthRange"; }
    inline StringView Desc()		{ return "description"; }
    inline StringView Distribution()	{ return "distribution"; }
    inline StringView Error()		{ return "error"; }
    inline StringView Examples()	{ return "examples"; }
    inline StringView Factor()		{ return "factor"; }
    inline StringView FileName()	{ return "fileName"; }
    inline StringView Filter()		{ return "filter"; }
    inline StringView FirstCrl()	{ return "firstCrossline"; }
    inline StringView FirstInl()	{ return "firstInline"; }
    inline StringView FirstTrc()	{ return "firstTrace"; }
    inline StringView FirstZ()		{ return "firstZ"; }
    inline StringView Font()		{ return "font"; }
    inline StringView Format()		{ return "format"; }
    inline StringView Formula()		{ return "formula"; }
    inline StringView GeomID()		{ return "geomID"; }
    inline StringView GeomIDs()		{ return "geomIDs"; }
    inline StringView GeomSystem()	{ return "geomSystem"; }
    inline StringView Geometry()	{ return "geometry"; }
    inline StringView Histogram()	{ return "histogram"; }
    inline StringView Hostname()	{ return "hostName"; }
    inline StringView ID()		{ return "id"; }
    inline StringView IDs()		{ return "ids"; }
    inline StringView InlRange()	{ return "inlineRange"; }
    inline StringView Inline()		{ return "inline"; }
    inline StringView Input()		{ return "input"; }
    inline StringView IPAddress()	{ return "ipAddress"; }
    inline StringView Is2D()		{ return "is2D"; }
    inline StringView IsLink()		{ return "isLink"; }
    inline StringView Keys()		{ return "keys"; }
    inline StringView LastCrl()		{ return "lastCrossline"; }
    inline StringView LastInl()		{ return "lastInline"; }
    inline StringView LastTrc()		{ return "lastTrace"; }
    inline StringView LastZ()		{ return "lastZ"; }
    inline StringView LatLong()		{ return "latLong"; }
    inline StringView Level()		{ return "level"; }
    inline StringView LineName()	{ return "lineName"; }
    inline StringView LineNames()	{ return "lineNames"; }
    inline StringView LineStyle()	{ return "lineStyle"; }
    inline StringView LogFile()		{ return "logFile"; }
    inline StringView MD()		{ return "md"; }
    inline StringView Marker()		{ return "marker"; }
    inline StringView Markers()		{ return "markers"; }
    inline StringView MarkerStyle()	{ return "markerStyle"; }
    inline StringView Maximum()		{ return "maximum"; }
    inline StringView Median()		{ return "median"; }
    inline StringView Minimum()		{ return "minimum"; }
    inline StringView Mnemonics()	{ return "mnemonics"; }
    inline StringView Mode()		{ return "mode"; }
    inline StringView Model()		{ return "model"; }
    inline StringView Models()		{ return "models"; }
    inline StringView Name()		{ return "name"; }
    inline StringView Names()		{ return "names"; }
    inline StringView NrFaults()	{ return "nrFaults";}
    inline StringView NrGeoms()		{ return "nrGeometries";}
    inline StringView NrItems()		{ return "nrItems";}
    inline StringView NrPicks()		{ return "nrPicks"; }
    inline StringView NrValues()	{ return "nrValues"; }
    inline StringView Object()		{ return "object"; }
    inline StringView Offset()		{ return "offset"; }
    inline StringView OffsetRange()	{ return "offsetRange"; }
    inline StringView Order()		{ return "order"; }
    inline StringView Output()		{ return "output"; }
    inline StringView OutputID()	{ return "outputID"; }
    inline StringView Pars()		{ return "parameters"; }
    inline StringView Position()	{ return "position"; }
    inline StringView Property()	{ return "property"; }
    inline StringView RMS()		{ return "rms"; }
    inline StringView Range()		{ return "range"; }
    inline StringView Region()		{ return "region"; }
    inline StringView RevZ()		{ return "reverseZ"; }
    inline StringView Sampling()	{ return "sampling"; }
    inline StringView Scale()		{ return "scale"; }
    inline StringView Scaling()		{ return "scaling"; }
    inline StringView Selection()	{ return "selection"; }
    inline StringView Server()		{ return "server"; }
    inline StringView ServerNm()	{ return "serverName"; }
    inline StringView Setup()		{ return "setup"; }
    inline StringView Shape()		{ return "shape"; }
    inline StringView Shortcuts()	{ return "shortcuts"; }
    inline StringView Shotpoint()	{ return "shotpoint"; }
    inline StringView Size()		{ return "size"; }
    inline StringView SliceType()	{ return "sliceType"; }
    inline StringView Source()		{ return "source"; }
    inline StringView StackOrder()	{ return "stackOrder"; }
    inline StringView Stats()		{ return "stats"; }
    inline StringView Status()		{ return "status"; }
    inline StringView StdDev()		{ return "stdDev"; }
    inline StringView StepCrl()		{ return "stepCrossline"; }
    inline StringView StepInl()		{ return "stepInline"; }
    inline StringView StepOut()		{ return "stepout"; }
    inline StringView StepOutCrl()	{ return "stepoutCrossline"; }
    inline StringView StepOutInl()	{ return "stepoutInline"; }
    inline StringView StepTrc()		{ return "stepTrace"; }
    inline StringView StepZ()		{ return "stepZ"; }
    inline StringView StratRef()	{ return "stratLevel"; }
    inline StringView Sum()		{ return "sum"; }
    inline StringView Survey()		{ return "survey"; }
    inline StringView SurveyID()	{ return "surveyID"; }
    inline StringView TVD()		{ return "tvd"; }
    inline StringView TVDSS()		{ return "tvdss"; }
    inline StringView TWT()		{ return "twt"; }
    inline StringView Target()		{ return "target"; }
    inline StringView Targets()		{ return "targets"; }
    inline StringView TermEm()		{ return "terminalEmulator"; }
    inline StringView Thickness()	{ return "thickness"; }
    inline StringView Time()		{ return "time"; }
    inline StringView TimeRange()	{ return "timeRange"; }
    inline StringView Title()		{ return "title"; }
    inline StringView TmpStor()		{ return "temporaryStorageLocation"; }
    inline StringView TraceKey()	{ return "traceKey"; }
    inline StringView TraceNr()		{ return "traceNumber"; }
    inline StringView Transparency()	{ return "transparency"; }
    inline StringView TrcRange()	{ return "traceRange"; }
    inline StringView Type()		{ return "type"; }
    inline StringView Types()		{ return "types"; }
    inline StringView Unit()		{ return "unit"; }
    inline StringView Units()		{ return "units"; }
    inline StringView Unscale()		{ return "unScale"; }
    inline StringView URI()		{ return "uri"; }
    inline StringView URL()		{ return "url"; }
    inline StringView User()		{ return "user"; }
    inline StringView Value()		{ return "value"; }
    inline StringView ValueRange()	{ return "valueRange"; }
    inline StringView Values()		{ return "values"; }
    inline StringView Variance()	{ return "variance"; }
    inline StringView Velocity()	{ return "velocity"; }
    inline StringView Version()		{ return "version"; }
    inline StringView Volume()		{ return "volume"; }
    inline StringView WaveletID()	{ return "waveletID"; }
    inline StringView Weight()		{ return "weight"; }
    inline StringView Well()		{ return "well"; }
    inline StringView X()		{ return "x"; }
    inline StringView XCoord()		{ return "xCoord"; }
    inline StringView XCoordinate()	{ return "xCoordinate"; }
    inline StringView XCoords()		{ return "xCoords"; }
    inline StringView XOffset()		{ return "xOffset"; }
    inline StringView XRange()		{ return "xRange"; }
    inline StringView Y()		{ return "y"; }
    inline StringView Y2()		{ return "y2"; }
    inline StringView YCoord()		{ return "yCoord"; }
    inline StringView YCoordinate()	{ return "yCoordinate"; }
    inline StringView YCoords()		{ return "yCoords"; }
    inline StringView YOffset()		{ return "yOffset"; }
    inline StringView YRange()		{ return "yRange"; }
    inline StringView Z()		{ return "z"; }
    inline StringView ZCoord()		{ return "zCoord"; }
    inline StringView ZDomain()		{ return "zDomain"; }
    inline StringView ZRange()		{ return "zRange"; }
    inline StringView ZSlice()		{ return "zSlice"; }
    inline StringView ZStep()		{ return "zStep"; }
    inline StringView ZUnit()		{ return "zUnit"; }
    inline StringView ZValue()		{ return "zValue"; }

    // History of objects
    inline StringView CrBy()		{ return "createdBy"; }
    inline StringView CrAt()		{ return "createdAt"; }
    inline StringView CrFrom()		{ return "createdFrom"; }
    inline StringView CrInfo()		{ return "createdInfo"; }
    inline StringView ModBy()		{ return "modifiedBy"; }
    inline StringView ModAt()		{ return "modifiedAt"; }
};
