/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/

static const char* rcsID = "$Id: uisynthtorealscale.cc,v 1.4 2011-02-09 12:29:17 cvsbert Exp $";

#include "uisynthtorealscale.h"
#include "uistratseisevent.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uihistogramdisplay.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uilabel.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisbuf.h"
#include "stratlevel.h"
#include "statruncalc.h"
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

uiSynthToRealScaleStatsDisp( uiParent* p, const char* nm, bool left )
    : uiGroup(p,nm)
    , usrval_(mUdf(float))
    , usrValChanged(this)
{
    uiFunctionDisplay::Setup su;
    su.annot( false ).noyaxis( true ).noy2axis( true ).drawgridlines( false );
    dispfld_ = new uiHistogramDisplay( this, su );
    dispfld_->setPrefWidth( 250 );
    dispfld_->setPrefHeight( GetGoldenMinor(250) );
    avgfld_ = new uiGenInput( this, "", FloatInpSpec() );
    if ( left )
	avgfld_->attach( rightAlignedBelow, dispfld_ );
    else
	avgfld_->attach( ensureBelow, dispfld_ );
    avgfld_->valuechanged.notify(mCB(this,uiSynthToRealScaleStatsDisp,avgChg));
    setHAlignObj( dispfld_ );
}

void avgChg( CallBacker* )
{
    usrval_ = avgfld_->getfValue();
    dispfld_->setMarkValue( usrval_, true );
    usrValChanged.trigger();
}

    float		usrval_;

    uiHistogramDisplay*	dispfld_;
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

    const IOObjContext horctxt( is2d_ ? mIOObjContext(EMHorizon2D)
	    			      : mIOObjContext(EMHorizon3D) );
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

    uiPushButton* gobut = new uiPushButton( this, "Extract values",
	    			mCB(this,uiSynthToRealScale,goPush), true );
    gobut->attach( alignedBelow, evfld_ );

    uiSeparator* sep = new uiSeparator( this, "separator" );
    sep->attach( stretchedBelow, gobut );

    uiGroup* statsgrp = new uiGroup( this, "Stats displays" );

    synthstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Synthetics",
	    					      true );
    realstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Real Seismics",
	    					     false );
    realstatsfld_->attach( rightOf, synthstatsfld_ );
    const CallBack setsclcb( mCB(this,uiSynthToRealScale,setScaleFld) );
    synthstatsfld_->usrValChanged.notify( setsclcb );
    realstatsfld_->usrValChanged.notify( setsclcb );
    statsgrp->attach( ensureBelow, sep );
    statsgrp->setHAlignObj( realstatsfld_ );

    finalscalefld_ = new uiGenInput( this, "", FloatInpSpec() );
    finalscalefld_->attach( centeredBelow, statsgrp );
    new uiLabel( this, "Scaling factor", finalscalefld_ );

    IOObjContext wvltctxt( mIOObjContext(Wavelet) );
    wvltctxt.forread = false;
    uiIOObjSel::Setup wvltsu( "Save scaled Wavelet" ); wvltsu.optional( true );
    wvltfld_ = new uiIOObjSel( this, wvltctxt, wvltsu );
    wvltfld_->attach( alignedBelow, finalscalefld_ );

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
	finalscalefld_->setValue( mUdf(float) );
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
    TypeSet<float> vals;
    if ( evfld_->getFromScreen() )
    {
	const Strat::SeisEvent& ssev = evfld_->event();
	for ( int idx=0; idx<synth_.size(); idx++ )
	{
	    const SeisTrc& trc = *synth_.get( idx );
	    const float reftm = ssev.snappedTime( trc );
	    if ( !mIsUdf(reftm) )
		vals += trc.getValue( reftm, 0 );
	}
    }
    synthstatsfld_->dispfld_->putN();
    synthstatsfld_->dispfld_->setData( vals.arr(), vals.size() );
    synthstatsfld_->avgfld_->setValue(
	    	synthstatsfld_->dispfld_->getRunCalc().average() );
}


void uiSynthToRealScale::updRealStats()
{
}
