/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uit2dconvsel.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uicombobox.h"
#include "ioobj.h"
#include "survinfo.h"
#include "veldesc.h"
#include "zdomain.h"

uiT2DConvSel::Setup::Setup( uiIOObjSel* tied, bool opt )
    : tiedto_(tied)
    , optional_(opt)
    , ist2d_(SI().zIsTime())
{
}


uiT2DConvSel::uiT2DConvSel( uiParent* p, const Setup& su )
    : uiZAxisTransformSel(p, su.optional_,
	su.ist2d_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth(),
	!su.ist2d_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth(),
	false, false )
    , setup_(su)
{
    uiString fldtext;
    if ( setup_.fldtext_.isEmpty() )
    {
	const StringView zunit = setup_.ist2d_
	    ? sKey::Depth()
	    : sKey::Time();

	if ( setup_.optional_ )
	    fldtext = tr( "Convert to %1" ).arg( zunit );
	else
	    fldtext = tr( "%1 conversion").arg( zunit );
    }

    setLabel( fldtext );

    if ( setup_.tiedto_ )
    {
	const CallBack cb( mCB(this,uiT2DConvSel,inpSel) );
	setup_.tiedto_->selectionDone.notify( cb );
	postFinalize().notify( cb );
    }
}


#define mGetGroupIdx \
    const int grpidx = choicefld_->currentItem() - (setup_.optional_ ? 1 : 0)


void uiT2DConvSel::inpSel( CallBacker* cb )
{
    if ( !setup_.tiedto_ ) return;
    const IOObj* ioobj = setup_.tiedto_->ioobj( true );
    if ( !ioobj ) return;

    selfld_->setSensitive( ZDomain::isSI(ioobj->pars()) );
}
