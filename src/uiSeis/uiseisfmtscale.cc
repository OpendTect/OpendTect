/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseisfmtscale.cc,v 1.12 2004-09-07 16:24:01 bert Exp $
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


uiSeisFmtScale::uiSeisFmtScale( uiParent* p, bool wfmt )
	: uiGroup(p,"Seis format and scale")
	, imptypefld(0)
	, optimfld(0)
	, is2d(false)
	, issteer(false)
{
    if ( wfmt )
	imptypefld = new uiGenInput( this, "Storage",
		     StringListInpSpec(DataCharacteristics::UserTypeNames) );

    scalefld = new uiScaler( this, 0, true );
    if ( imptypefld )
	scalefld->attach( alignedBelow, imptypefld );

    if ( wfmt )
    {
	optimfld = new uiGenInput( this, "Optimize horizontal slice access",
				   BoolInpSpec() );
	optimfld->setValue( false );
	optimfld->attach( alignedBelow, scalefld );
    }

    setHAlignObj( scalefld );
    mainwin()->finaliseDone.notify( mCB(this,uiSeisFmtScale,updFldsForType) );
}


void uiSeisFmtScale::setSteering( bool yn )
{
    issteer = yn;
    updFldsForType(0);
}


void uiSeisFmtScale::set2D( bool yn )
{
    is2d = yn;
    updFldsForType(0);
}


void uiSeisFmtScale::updFldsForType( CallBacker* )
{
    if ( issteer )
    {
	scalefld->setUnscaled();
	if ( imptypefld ) imptypefld->setValue( 0 );
    }
    scalefld->setSensitive( !issteer );
    if ( optimfld ) optimfld->display( !is2d );
    if ( imptypefld )
    {
	if ( is2d )
	    imptypefld->display( false );
	else
	    imptypefld->setSensitive( !issteer );
    }
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
    return !is2d && optimfld ? optimfld->getBoolValue() : false;
}


void uiSeisFmtScale::updateFrom( const IOObj& ioobj )
{
    const char* res = ioobj.pars().find( "Type" );
    setSteering( res && *res == 'S' );
    if ( imptypefld )
    {
	const char* res = ioobj.pars().find( "Data storage" );
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
    if ( !ioobj || is2d ) return;

    const int tp = getFormat();
    ioobj->pars().set( "Data storage", DataCharacteristics::UserTypeNames[tp] );
    ioobj->pars().removeWithKey( "Optimized direction" );
    if ( horOptim() )
	ioobj->pars().set( "Optimized direction", "Horizontal" );

    IOM().to( ioobj->key() );
    IOM().commitChanges( *ioobj );
}
