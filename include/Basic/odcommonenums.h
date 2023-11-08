#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "enums.h"


namespace OD
{

/*!\brief Fundamental orientation */

enum Orientation
{
    Horizontal=0,
    Vertical=1
};


/*!\brief OpendTect flat slice types */

enum SliceType
{
    InlineSlice=0,
    CrosslineSlice=1,
    ZSlice=2
};


/*!\brief What to choose from any list-type UI object */

enum ChoiceMode
{
    ChooseNone=0,
    ChooseOnlyOne=1,
    ChooseAtLeastOne=2,
    ChooseZeroOrMore=3
};


/*!\brief State of check objects */

enum CheckState
{
    Unchecked=0,
    PartiallyChecked=1,
    Checked=2
};


/*!\brief How to select files or directories */

enum FileSelectionMode
{
    SelectFileForRead,	/*!<The name of a single existing file. */
    SelectFileForWrite,	/*!<The name of a file, whether it exists or not. */
    SelectMultiFile,	/*!<The names of zero or more existing files. */
    SelectDirectory,	/*!<The name of a directory. */
    SelectDirectoryWithFile	//!<The name of a directory with a given file.
};


/*!\brief File content types, for which operations may be known. */

enum FileContentType
{
    GeneralContent=0,
    ImageContent,
    TextContent,
    HtmlContent
};


enum Edge
{
    Top=0,
    Left=1,
    Right=2,
    Bottom=3
};


enum Corner
{
    TopLeft=0,
    TopRight=1,
    BottomLeft=2,
    BottomRight=3
};


enum StdActionType
{
    NoIcon=0,
    Apply,
    Cancel,
    Define,
    Delete,
    Edit,
    Help,
    Ok,
    Options,
    Properties,
    Examine,
    Rename,
    Remove,
    Save,
    SaveAs,
    Select,
    Settings,
    Unload,
    Video
};

enum WindowActivationBehavior
{
    DefaultActivateWindow,
    AlwaysActivateWindow
};


enum WellType
{
    UnknownWellType=0,
    OilWell=1,
    GasWell=2,
    OilGasWell=3,
    DryHole=4,
    PluggedOilWell=5,
    PluggedGasWell=6,
    PluggedOilGasWell=7,
    PermittedLocation=8,
    CanceledLocation=9,
    InjectionDisposalWell=10
};

mDeclareNameSpaceEnumUtils(Basic,WellType);

} // namespace OD

namespace Seis
{

enum OffsetType
{
    OffsetMeter=0,
    OffsetFeet=1,
    AngleRadians=2,
    AngleDegrees=3
};

mDeclareNameSpaceEnumUtils(Basic,OffsetType)

mGlobal(Basic) bool isOffsetDist(OffsetType);
mGlobal(Basic) bool isOffsetAngle(OffsetType);

} // namespace Seis

namespace Vel
{

enum Type
{
    Unknown=0,
    Interval=1,
    RMS=2,
    Avg=3,
    Delta=4,
    Epsilon=5,
    Eta=6
};

mDeclareNameSpaceEnumUtils(Basic,Type)

} // namespace Vel

namespace ZDomain
{

enum DistType
{
    Meter=0,
    Feet=1
};

typedef DistType DepthType;
typedef DistType XYType;

} // namespace ZDomain


mGlobal(Basic) bool isHorizontal(OD::Orientation);
mGlobal(Basic) bool isVertical(OD::Orientation);
mGlobal(Basic) bool isMultiChoice(OD::ChoiceMode);
mGlobal(Basic) bool isOptional(OD::ChoiceMode);
