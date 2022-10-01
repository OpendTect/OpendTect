/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellwriteopts.h"
#include "settings.h"
#include "welllog.h"
#include "welltransl.h"
#include "wellodwriter.h"
#include "uigeninput.h"
#include "uistrings.h"

#define mODWellTranslInstance mTranslTemplInstance(Well,od)


uiODWellWriteOpts::uiODWellWriteOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mODWellTranslInstance)
    , defbinwrite_(true)
{
    mSettUse(getYN,"dTect.Well logs","Binary format",defbinwrite_);
    wrlogbinfld_ = new uiGenInput( this, tr("%1 storage")
		       .arg(uiStrings::sWellLog()), BoolInpSpec(defbinwrite_,
		       tr("Binary"),uiStrings::sASCII()) );

    setHAlignObj( wrlogbinfld_ );
}


uiODWellWriteOpts::~uiODWellWriteOpts()
{}


void uiODWellWriteOpts::use( const IOPar& iop )
{
    const BufferString res = iop.find( Well::odWriter::sKeyLogStorage() );
    bool binwr = defbinwrite_;
    if ( !res.isEmpty() )
	binwr = res.firstChar() != 'A';

    wrlogbinfld_->setValue( binwr );
}


bool uiODWellWriteOpts::fill( IOPar& iop ) const
{
    const bool wantbin = wrlogbinfld_->getBoolValue();
    iop.set( Well::odWriter::sKeyLogStorage(), wantbin ? "Binary" : "Ascii" );
    return true;
}


void uiODWellWriteOpts::initClass()
{
    factory().addCreator( create, mODWellTranslInstance.getDisplayName() );
}
