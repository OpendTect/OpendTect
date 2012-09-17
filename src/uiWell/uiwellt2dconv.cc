/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellt2dconv.cc,v 1.2 2010/07/08 06:07:30 cvsnageswara Exp $";

#include "uiwellt2dconv.h"
#include "uiioobjsel.h"
#include "welltransl.h"


uiT2DWellConvSelGroup::uiT2DWellConvSelGroup( uiParent* p )
    : uiT2DConvSelGroup(p,"Well T2D conv sel")
{
    fld_ = new uiIOObjSel( this, mIOObjContext(Well), "" );
}


bool uiT2DWellConvSelGroup::usePar( const IOPar& iop )
{
    const IOObj* ioobj = fld_->ioobj();
    MultiID ky; if ( ioobj ) ky = ioobj->key();
    if ( iop.get("ID",ky) )
	fld_->setInput( ky );

    return true;
}


bool uiT2DWellConvSelGroup::fillPar( IOPar& iop ) const
{
    const IOObj* ioobj = fld_->ioobj();
    if ( !ioobj )
	iop.removeWithKey("ID");
    else
	iop.set( "ID", ioobj->key() );

    return true;
}


void uiT2DWellConvSelGroup::initClass()
{
    uiT2DConvSelGroup::factory().addCreator( create, "Well" );
}
