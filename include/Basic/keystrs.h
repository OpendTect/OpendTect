#ifndef keystrs_h
#define keystrs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2002

 RCS:		$Id$
________________________________________________________________________

-*/
 
 
#include "gendefs.h"
#include "fixedstring.h"


#ifndef OD_EXT_KEYSTR_EXPAND
# define mKeyStrsNameSpace(ns) namespace ns
# define mKeyStrsDecl(nm,str) mBasicExtern FixedString nm
#endif



/*!\brief is used for defining key strings that are 'global'.

Some standard key strings are shared between otherwise unrelated modules.
To make sure no artificial dependencies are created, such a key can be added
to this namespace.

*/

mKeyStrsNameSpace(sKey)
{

    mKeyStrsDecl(Ascii,		"Ascii");
    mKeyStrsDecl(Attribute, 	"Attribute");
    mKeyStrsDecl(Attributes,	"Attributes");
    mKeyStrsDecl(Azimuth,	"Azimuth");
    mKeyStrsDecl(Binary,	"Binary");
    mKeyStrsDecl(Body,		"Body");
    mKeyStrsDecl(Color,		"Color");
    mKeyStrsDecl(Component,	"Component");
    mKeyStrsDecl(Content,	"Content");
    mKeyStrsDecl(Crossline,	"Cross-line");
    mKeyStrsDecl(Cube,		"Cube");
    mKeyStrsDecl(DataRoot,	"Data Root");
    mKeyStrsDecl(DataStorage,	"Data storage");
    mKeyStrsDecl(DataType,	"DataType");
    mKeyStrsDecl(Date,		"Date");
    mKeyStrsDecl(Depth,		"Depth");
    mKeyStrsDecl(Desc,		"Description");
    mKeyStrsDecl(EmptyString,	"");
    mKeyStrsDecl(Factor,	"Factor");
    mKeyStrsDecl(FileName,	"File name");
    mKeyStrsDecl(Filter,	"Filter");
    mKeyStrsDecl(FloatUdf,	"1e30");
    mKeyStrsDecl(Geometry,	"Geometry");
    mKeyStrsDecl(ID,		"ID");
    mKeyStrsDecl(IOSelection,	"I/O Selection");
    mKeyStrsDecl(Inline,	"In-line");
    mKeyStrsDecl(Keys,		"Keys");
    mKeyStrsDecl(LineKey,	"Line key");
    mKeyStrsDecl(LineName,	"Line name");
    mKeyStrsDecl(Log,		"Log");
    mKeyStrsDecl(LogFile,	"Log file");
    mKeyStrsDecl(Name,		"Name");
    mKeyStrsDecl(No,		"No");
    mKeyStrsDecl(None,		"None");
    mKeyStrsDecl(Offset,	"Offset");
    mKeyStrsDecl(Output,	"Output");
    mKeyStrsDecl(Pars,		"Parameters");
    mKeyStrsDecl(Polygon,	"Polygon");
    mKeyStrsDecl(Position,	"Position");
    mKeyStrsDecl(Property,	"Property");
    mKeyStrsDecl(Random,	"Random");
    mKeyStrsDecl(Range,		"Range");
    mKeyStrsDecl(Sampling,	"Sampling");
    mKeyStrsDecl(Scale,		"Scale");
    mKeyStrsDecl(Selection,	"Selection");
    mKeyStrsDecl(Subsample,	"Subsample");
    mKeyStrsDecl(Shortcuts,	"Shortcuts");
    mKeyStrsDecl(Size,		"Size");
    mKeyStrsDecl(SpaceString,	" ");
    mKeyStrsDecl(Steering,	"Steering");
    mKeyStrsDecl(Stored,	"Stored");
    mKeyStrsDecl(StratRef,	"Strat Level");
    mKeyStrsDecl(Subsel,	"Subsel");
    mKeyStrsDecl(Surface,	"Surface");
    mKeyStrsDecl(Survey,	"Survey");
    mKeyStrsDecl(Table,		"Table");
    mKeyStrsDecl(Target,	"Target");
    mKeyStrsDecl(Time,		"Time");
    mKeyStrsDecl(Title,		"Title");
    mKeyStrsDecl(TraceNr,	"Trace number");
    mKeyStrsDecl(Type,		"Type");
    mKeyStrsDecl(Undef,		"Undefined");
    mKeyStrsDecl(Unit,		"Unit");
    mKeyStrsDecl(Value,		"Value");
    mKeyStrsDecl(Version,	"Version");
    mKeyStrsDecl(Well,		"Well");
    mKeyStrsDecl(XCoord,	"X-Coord");
    mKeyStrsDecl(YCoord,	"Y-Coord");
    mKeyStrsDecl(Yes,		"Yes");

    mKeyStrsDecl(Average,	"Average");
    mKeyStrsDecl(Maximum,	"Maximum");
    mKeyStrsDecl(Median,	"Median");
    mKeyStrsDecl(Minimum,	"Minimum");
    mKeyStrsDecl(StdDev,	"StdDev");
    mKeyStrsDecl(Sum,		"Sum");

    mKeyStrsDecl(BinIDSel,	"BinID selection");
    mKeyStrsDecl(InlRange,	"In-line range");
    mKeyStrsDecl(FirstInl,	"First In-line");
    mKeyStrsDecl(LastInl,	"Last In-line");
    mKeyStrsDecl(StepInl,	"Step In-line");
    mKeyStrsDecl(StepOutInl,	"Stepout In-line");
    mKeyStrsDecl(CrlRange,	"Cross-line range");
    mKeyStrsDecl(FirstCrl,	"First Cross-line");
    mKeyStrsDecl(LastCrl,	"Last Cross-line");
    mKeyStrsDecl(StepCrl,	"Step Cross-line");
    mKeyStrsDecl(StepOutCrl,	"Stepout Cross-line");
    mKeyStrsDecl(ZRange,	"Z range");
    mKeyStrsDecl(FirstTrc,	"First Trace");
    mKeyStrsDecl(LastTrc,	"Last Trace");
    mKeyStrsDecl(TrcRange,	"Trace Range");

    mKeyStrsDecl(TmpStor,	"Temporary storage location");

    mKeyStrsDecl(Default,	"Default");
    mKeyStrsDecl(DefCube,	"Default.Cube");
    mKeyStrsDecl(DefLineSet,	"Default.LineSet");
    mKeyStrsDecl(DefLine,	"Default.Line");
    mKeyStrsDecl(DefAttribute,	"Default.Attribute");
    mKeyStrsDecl(DefPS3D,	"Default.PS3D Data Store");
    mKeyStrsDecl(DefPS2D,	"Default.PS2D Data Store");
    mKeyStrsDecl(DefWavelet,	"Default.Wavelet");

};

#undef mKeyStrsNameSpace
#undef mKeyStrsDecl

// Makes compat with 4.4 easier
#define mGetKeyStr(id)  sKey::id


#endif
