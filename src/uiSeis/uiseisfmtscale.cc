/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseisfmtscale.cc,v 1.17 2007-02-05 14:32:25 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiseisfmtscale.h"
#include "uiscaler.h"
#include "uimainwin.h"
#include "datachar.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"

#include "uigeninput.h"


uiSeisFmtScale::uiSeisFmtScale( uiParent* p, Seis::GeomType gt, bool forexp )
	: uiGroup(p,"Seis format and scale")
	, geom_(gt)
	, issteer_(false)
	, imptypefld(0)
	, optimfld(0)
{
    if ( !forexp && !Seis::is2D(geom_) )
	imptypefld = new uiGenInput( this, "Storage",
		     StringListInpSpec(DataCharacteristics::UserTypeNames) );

    scalefld = new uiScaler( this, 0, true );
    if ( imptypefld )
	scalefld->attach( alignedBelow, imptypefld );

    if ( imptypefld && !Seis::isPS(geom_) )
    {
	optimfld = new uiGenInput( this, "Optimize horizontal slice access",
				   BoolInpSpec(true) );
	optimfld->setValue( false );
	optimfld->attach( alignedBelow, scalefld );
    }

    setHAlignObj( scalefld );
    mainwin()->finaliseDone.notify( mCB(this,uiSeisFmtScale,updSteer) );
}


void uiSeisFmtScale::setSteering( bool yn )
{
    issteer_ = yn;
    updSteer( 0 );
}


void uiSeisFmtScale::updSteer( CallBacker* )
{
    if ( issteer_ )
    {
	scalefld->setUnscaled();
	if ( imptypefld ) imptypefld->setValue( 0 );
    }
    scalefld->setSensitive( !issteer_ );
    if ( imptypefld )
	imptypefld->setSensitive( !issteer_ );
}


Scaler* uiSeisFmtScale::getScaler() const
{
    return scalefld->getScaler();
}


int uiSeisFmtScale::getFormat() const
{
    return imptypefld ? imptypefld->getIntValue() : 0;
}


bool uiSeisFmtScale::horOptim() const
{
    return optimfld ? optimfld->getBoolValue() : false;
}


void uiSeisFmtScale::updateFrom( const IOObj& ioobj )
{
    const char* res = ioobj.pars().find( "Type" );
    setSteering( res && *res == 'S' );
    if ( imptypefld )
    {
	res = ioobj.pars().find( "Data storage" );
	if ( res )
	    imptypefld->setValue( (int)*res );
    }
    if ( optimfld )
    {
	res = ioobj.pars().find( "Optimized direction" );
	if ( res && *res == 'H' )
	    optimfld->setValue( true );
    }
}


void uiSeisFmtScale::updateIOObj( IOObj* ioobj ) const
{
    if ( !ioobj || Seis::is2D(geom_) ) return;

    const int tp = getFormat();
    ioobj->pars().set( "Data storage", DataCharacteristics::UserTypeNames[tp] );
    ioobj->pars().removeWithKey( "Optimized direction" );
    if ( horOptim() )
	ioobj->pars().set( "Optimized direction", "Horizontal" );

    IOM().to( ioobj->key() );
    IOM().commitChanges( *ioobj );
}
