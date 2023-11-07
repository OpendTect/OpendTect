/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odcommonenums.h"

mDefineNameSpaceEnumUtils(OD,WellType,"Well Type")
{
    "Unknown",
    "OilWell",
    "GasWell",
    "OilGasWell",
    "DryHole",
    "PluggedOilWell",
    "PluggedGasWell",
    "PluggedOilGasWell",
    "PermittedLocation",
    "CanceledLocation",
    "InjectionDisposalWell",
    nullptr
};


template <>
void EnumDefImpl<OD::WellType>::init()
{
    uistrings_ += tr("Unknown");
    uistrings_ += tr("Oil Well");
    uistrings_ += tr("Gas Well");
    uistrings_ += tr("Oil and Gas Well");
    uistrings_ += tr("Dry Hole");
    uistrings_ += tr("Plugged Oil Well");
    uistrings_ += tr("Plugged Gas Well");
    uistrings_ += tr("Plugged Oil and Gas Well");
    uistrings_ += tr("Permitted Location");
    uistrings_ += tr("Canceled Location");
    uistrings_ += tr("Injection Disposal Well");

    iconfiles_.add( "unknownwelltype" );
    iconfiles_.add( "oilwell" );
    iconfiles_.add( "gaswell" );
    iconfiles_.add( "oilgaswell" );
    iconfiles_.add( "dryhole" );
    iconfiles_.add( "pluggedoilwell" );
    iconfiles_.add( "pluggedgaswell" );
    iconfiles_.add( "pluggedoilgaswell" );
    iconfiles_.add( "permittedlocation" );
    iconfiles_.add( "canceledlocation" );
    iconfiles_.add( "injectiondisposalwell" );
}

bool isHorizontal( OD::Orientation orient )
{
    return orient == OD::Horizontal;
}


bool isVertical( OD::Orientation orient )
{
    return orient == OD::Vertical;
}


bool isMultiChoice( OD::ChoiceMode cm )
{
    return cm > OD::ChooseOnlyOne;
}


bool isOptional( OD::ChoiceMode cm )
{
    return cm == OD::ChooseZeroOrMore;
}


mDefineNameSpaceEnumUtils(Seis,OffsetType,"Offset Type")
{
    "Offset in meters",
    "Offset in feet",
    "Angle in radians",
    "Angle in degrees",
    nullptr
};


bool Seis::isOffsetDist( OffsetType typ )
{
    return typ == OffsetMeter || typ == OffsetFeet;
}


bool Seis::isOffsetAngle( OffsetType typ )
{
    return typ == AngleRadians || typ == AngleDegrees;
}


mDefineNameSpaceEnumUtils(Vel,Type,"Velocity Type")
{
    "Unknown",
    "Vint",
    "Vrms",
    "Vavg",
    "Delta",
    "Epsilon",
    "Eta",
    nullptr
};
