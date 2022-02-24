/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Feb 2022
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
    0
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
