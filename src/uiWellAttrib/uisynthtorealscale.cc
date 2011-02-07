/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID = "$Id: uisynthtorealscale.cc,v 1.1 2011-02-07 10:25:11 cvsbert Exp $";

#include "uisynthtorealscale.h"
#include "uistratseisevent.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisbuf.h"

#include "uimsg.h"


uiSynthToRealScale::uiSynthToRealScale( uiParent* p, SeisTrcBuf& tb,
					const MultiID& wid, const char* lvlnm )
    : uiDialog(p,Setup("Scale synthetics","Determine scaling for synthetics",
			mTODOHelpID))
    , synth_(tb)
    , wvltid_(wid)
{
    evfld_ = new uiStratSeisEvent( this, true );

    finaliseDone.notify( mCB(this,uiSynthToRealScale,initWin) );
}


void uiSynthToRealScale::initWin( CallBacker* )
{
}


bool uiSynthToRealScale::acceptOK( CallBacker* )
{
    if ( !evfld_->getFromScreen() )
	return false;

    SeisTrc& trc = *synth_.get( 0 );
    SeisTrcPropChg tpc( *synth_.get(synth_.size()/2) );
    tpc.scale( 3 );
    return true;
}
