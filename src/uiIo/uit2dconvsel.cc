/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uit2dconvsel.h"

#include "uigeninput.h"
#include "uiioobjsel.h"

#include "ioobj.h"
#include "survinfo.h"
#include "zdomain.h"

uiT2DConvSel::Setup::Setup( uiIOObjSel* tied, bool opt, bool is2d )
    : tiedto_(tied)
    , optional_(opt)
    , ist2d_(SI().zIsTime())
    , is2d_(is2d)
{
}


uiT2DConvSel::Setup::~Setup()
{}


uiT2DConvSel::uiT2DConvSel( uiParent* p, const Setup& su )
    : uiZAxisTransformSel(p, su.optional_,
	su.ist2d_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth(),
	!su.ist2d_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth(),
	false, false, su.is2d_ )
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
	mAttachCB( setup_.tiedto_->selectionDone, uiT2DConvSel::inpSel );
	mAttachCB( postFinalize(), uiT2DConvSel::inpSel );
    }
}


uiT2DConvSel::~uiT2DConvSel()
{
    detachAllNotifiers();
}


void uiT2DConvSel::inpSel( CallBacker* cb )
{
    if ( !setup_.tiedto_ )
	return;

    const IOObj* ioobj = setup_.tiedto_->ioobj( true );
    if ( !ioobj )
	return;

    selfld_->setSensitive( ZDomain::isSI(ioobj->pars()) );
}
