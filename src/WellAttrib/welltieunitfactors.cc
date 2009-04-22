/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieunitfactors.cc,v 1.2 2009-04-22 09:22:06 cvsbruno Exp $";

#include "welltieunitfactors.h"

#include "math.h"
#include "unitofmeasure.h"
#include "welllog.h"
#include "welldata.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltiesetup.h"


WellTieUnitFactors::WellTieUnitFactors( const WellTieSetup* wtsetup )
	    : velfactor_(0)
	    , denfactor_(0)
	    , veluom_(0)		    
	    , denuom_(0)
{
    if ( !wtsetup ) return; 

    Well::Data* wd = Well::MGR().get( wtsetup->wellid_ ); 
    if ( !wd ) return;; 

    const Well::Log* vellog =  wd->logs().getLog( wtsetup->vellognm_ );
    const Well::Log* denlog =  wd->logs().getLog( wtsetup->denlognm_ );
    if ( !vellog || !denlog ) 
	{ pErrMsg("Unable to access logs"); return; }

    const char* umlden = denlog->unitMeasLabel();
    denuom_ = UnitOfMeasure::getGuessed( umlden );
    const char* umlval = vellog->unitMeasLabel();
    veluom_ = UnitOfMeasure::getGuessed( umlval );

    calcVelFactor( veluom_->symbol(), wtsetup->issonic_ );
    calcDensFactor( denuom_->symbol() );

    Well::MGR().release( wtsetup->wellid_ ); 
}


void WellTieUnitFactors::calcVelFactor( const char* velunit,
       						bool issonic )
{
    issonic ? calcSonicVelFactor( velunit ) : calcVelFactor( velunit );
}


void WellTieUnitFactors::calcSonicVelFactor( const char* velunit )
{
    double velfactor;
    if ( !strcmp( velunit, "None") )
	velfactor = 0.001*0.3048;
    if ( !strcmp( velunit, "us/ft") )
	velfactor =  0.001*0.3048;
    else if ( !strcmp(velunit, "ms/ft" ) )
	velfactor =  1*0.3048;
    else if ( !strcmp( velunit, "s/ft" ) )
	velfactor =  1000*0.3048;
    else if ( !strcmp( velunit, "us/m") )
	velfactor =  0.001;
    else if ( !strcmp( velunit, "ms/m") )
	velfactor =  1;
    else if ( !strcmp( velunit, "s/m") )
	velfactor =  1000;
    else
	velfactor = 0.001*0.3048; // us/ft taken if no unit found

    velfactor_ = velfactor;
}


void WellTieUnitFactors::calcVelFactor( const char* velunit )
{
    double velfactor;
    if ( !strcmp( velunit, "None") )
	velfactor = 0.001/0.3048;
    if ( !strcmp( velunit, "ft/us") )
	velfactor =  0.001/0.3048;
    else if ( velunit, "ft/ms")
	velfactor =  1/0.3048;
    else if ( !strcmp( velunit, "ft/s" ) )
	velfactor =  1000/0.3048;
    else if ( !strcmp( velunit, "m/us") )
	velfactor =  1/0.001;
    else if ( !strcmp( velunit, "m/ms") )
	velfactor =  1;
    else if ( !strcmp( velunit, "m/s") )
	velfactor =  1/1000;
    else
	velfactor = 0.001/0.3048; // ft/us taken if no unit found

    velfactor_ = velfactor;
}


void WellTieUnitFactors::calcDensFactor( const char* densunit )
{
    double denfactor;
    if ( !strcmp( densunit, "None") )
	denfactor = 1000;
    if ( !strcmp( densunit, "g/cc") )
	denfactor =  1000;
    else if ( !strcmp( densunit, "g/cm") )
	denfactor =  0.001;
    else if ( !strcmp( densunit, "kg/cc") )
	denfactor =  1000000;
    else if ( !strcmp( densunit, "kg/cm") )
	denfactor =  1;
    else
	denfactor = 1000; // g/cc taken if no unit found

    denfactor_ = denfactor;
}



