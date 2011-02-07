/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID = "$Id: uisynthtorealscale.cc,v 1.2 2011-02-07 16:17:43 cvsbert Exp $";

#include "uisynthtorealscale.h"
#include "uistratseisevent.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uistatsdisplay.h"
#include "uigeninput.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisbuf.h"
#include "stratlevel.h"
#include "picksettr.h"
#include "pickset.h"
#include "polygon.h"
#include "wavelet.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emsurfacetr.h"
#include "emsurfaceposprov.h"

#include "uimsg.h"


class uiSynthToRealScaleStatsDisp : public uiGroup
{
public:

uiSynthToRealScaleStatsDisp( uiParent* p, const char* nm )
    : uiGroup(p,nm)
    , usrval_(mUdf(float))
    , usrValChanged(this)
{
    uiStatsDisplay::Setup su; su.withtext( false );
    dispfld_ = new uiStatsDisplay( this, su );
    dispfld_->setPrefWidth( 250 );
    dispfld_->setPrefHeight( GetGoldenMinor(250) );
    avgfld_ = new uiGenInput( this, "", FloatInpSpec() );
    avgfld_->attach( ensureBelow, dispfld_ );
    avgfld_->valuechanged.notify(mCB(this,uiSynthToRealScaleStatsDisp,avgChg));
}

void avgChg( CallBacker* )
{
    usrval_ = avgfld_->getfValue();
    dispfld_->setMarkValue( usrval_, true );
    usrValChanged.trigger();
}

    float		usrval_;

    uiStatsDisplay*	dispfld_;
    uiGenInput*		avgfld_;

    Notifier<uiSynthToRealScaleStatsDisp>	usrValChanged;

};


uiSynthToRealScale::uiSynthToRealScale( uiParent* p, bool is2d, SeisTrcBuf& tb,
					const MultiID& wid, const char* lvlnm )
    : uiDialog(p,Setup("Scale synthetics","Determine scaling for synthetics",
			mTODOHelpID))
    , is2d_(is2d)
    , synth_(tb)
    , wvltid_(wid)
{
    uiSeisSel::Setup sssu( is2d_, false );
    seisfld_ = new uiSeisSel( this, uiSeisSel::ioContext(sssu.geom_,true),
	    		      sssu );

    const IOObjContext horctxt( is2d_ ? mIOObjContext(EMHorizon3D)
	    			      : mIOObjContext(EMHorizon2D) );
    uiIOObjSel::Setup horsu( BufferString("Horizon for '",lvlnm,"'") );
    horfld_ = new uiIOObjSel( this, horctxt, horsu );
    horfld_->attach( alignedBelow, seisfld_ );

    IOObjContext polyctxt( mIOObjContext(PickSet) );
    polyctxt.toselect.require_.set( sKey::Type, sKey::Polygon );
    uiIOObjSel::Setup polysu( "Within Polygon" ); polysu.optional( true );
    polyfld_ = new uiIOObjSel( this, polyctxt, polysu );
    polyfld_->attach( alignedBelow, horfld_ );

    uiStratSeisEvent::Setup ssesu( true );
    ssesu.fixedlevel( Strat::LVLS().get(lvlnm) );
    evfld_ = new uiStratSeisEvent( this, ssesu );
    evfld_->attach( alignedBelow, polyfld_ );

    uiSeparator* sep = new uiSeparator( this, "separator" );
    sep->attach( stretchedBelow, evfld_ );

    uiGroup* statsgrp = new uiGroup( this, "Stats displays" );
    statsgrp->attach( ensureBelow, sep );

    synthstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Synthetics" );
    realstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Real Seismics");
    realstatsfld_->attach( rightOf, synthstatsfld_ );
    const CallBack setsclcb( mCB(this,uiSynthToRealScale,setScaleFld) );
    synthstatsfld_->usrValChanged.notify( setsclcb );
    realstatsfld_->usrValChanged.notify( setsclcb );

    finalscalefld_ = new uiGenInput( this, "Scale to apply", FloatInpSpec() );
    finalscalefld_->attach( centeredBelow, statsgrp );

    finaliseDone.notify( mCB(this,uiSynthToRealScale,initWin) );
}


void uiSynthToRealScale::initWin( CallBacker* )
{
    updSynthStats();
}


void uiSynthToRealScale::setScaleFld( CallBacker* )
{
    const float synthval = synthstatsfld_->usrval_;
    const float realval = realstatsfld_->usrval_;
    if ( mIsUdf(synthval) || mIsUdf(realval) || synthval == 0 )
	finalscalefld_->setValue( synthval );
    else
	finalscalefld_->setValue( realval / synthval );
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


void uiSynthToRealScale::updSynthStats()
{
}


void uiSynthToRealScale::updRealStats()
{
}
