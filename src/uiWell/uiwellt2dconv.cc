/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwellt2dconv.h"
#include "uiioobjsel.h"
#include "welltransl.h"
#include "keystrs.h"


uiT2DWellConvSelGroup::uiT2DWellConvSelGroup( uiParent* p )
    : uiT2DConvSelGroup(p,"Well T2D conv sel")
{
    fld_ = new uiIOObjSel( this, mIOObjContext(Well), "" );
}


bool uiT2DWellConvSelGroup::usePar( const IOPar& iop )
{
    const IOObj* ioobj = fld_->ioobj(true);
    MultiID ky; if ( ioobj ) ky = ioobj->key();

    const bool havekey = iop.get( sKey::ID(), ky );
    if ( havekey )
	fld_->setInput( ky );

    return havekey;
}


bool uiT2DWellConvSelGroup::fillPar( IOPar& iop ) const
{
    const IOObj* ioobj = fld_->ioobj();
    if ( !ioobj )
	iop.removeWithKey("ID");
    else
	iop.set( "ID", ioobj->key() );

    return (bool)ioobj;
}


void uiT2DWellConvSelGroup::initClass()
{
    uiT2DConvSelGroup::factory().addCreator( create, "Well" );
}
