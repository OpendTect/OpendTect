/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : Mar 2018
-*/


#include "uivolprocudfreplacer.h"

#include "keystrs.h"
#include "volprocudfreplacer.h"

#include "uigeninput.h"


VolProc::uiUdfValReplacer::uiUdfValReplacer( uiParent* p, UdfReplacer* replacer,
					     bool is2d )
    : uiStepDialog(p,UdfReplacer::sFactoryDisplayName(),replacer,is2d)
    , replacer_( replacer )
{
    setHelpKey( mODHelpKey(mVolProcUdfReplacerHelpID) );

    IOPar par;
    if ( replacer_ ) replacer_->fillPar( par );
    float replval = 0.f;
    par.get( sKey::Value(), replval );
    replvalfld_ = new uiGenInput( this, tr("Replacement value"),
				  FloatInpSpec(replval) );


    bool dopad = false;
    par.getYN( VolProc::UdfReplacer::sKeyPadTraces(), dopad );
    padtrcsfls_ = new uiGenInput( this, tr("Pad missing traces"),
				  BoolInpSpec(dopad) );
    padtrcsfls_->attach( alignedBelow, replvalfld_ );

    addNameFld( padtrcsfls_ );
}


VolProc::uiUdfValReplacer::~uiUdfValReplacer()
{
}


VolProc::uiStepDialog* VolProc::uiUdfValReplacer::createInstance(
							 uiParent* parent,
							 Step* ps, bool is2d )
{
    mDynamicCastGet( UdfReplacer*, replacer, ps );
    if ( !replacer ) return 0;

    return new uiUdfValReplacer( parent, replacer, is2d );
}


bool VolProc::uiUdfValReplacer::acceptOK()
{
    if ( !uiStepDialog::acceptOK() )
	return false;

    IOPar par;
    par.set( sKey::Value(), replvalfld_->getFValue() );
    par.setYN( VolProc::UdfReplacer::sKeyPadTraces(),
	       padtrcsfls_->getBoolValue() );

    return replacer_->usePar( par );
}
