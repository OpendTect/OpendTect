/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "";

#include "uigainanalysisdlg.h"

#include "uiaxishandler.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uispinbox.h"

#include "axislayout.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "survinfo.h"

uiGainAnalysisDlg::uiGainAnalysisDlg( uiParent* p, const SeisTrcBuf& traces,
      TypeSet<float>& zvals, TypeSet<float>& scalefac )
    : uiDialog(p,uiDialog::Setup("Analyse Gain","",""))
    , zvals_(zvals)
    , scalefactors_(scalefac)
    , trcbuf_(traces)
{
    convertZTo( false );

    Interval<float> scalerg( mUdf(float), -mUdf(float) );
    for ( int idx=0; idx<scalefac.size(); idx++ )
	scalerg.include( scalefac[idx], false );

    if ( !scalefac.size() )
	scalerg = Interval<float>(0,100);

    AxisLayout<float> al( scalerg );
    scalerg = Interval<float>( al.sd_.start, al.stop_ );

    SamplingData<float> zsd = trcbuf_.get(0)->info().sampling;
    Interval<float> zrg( zsd.start, zsd.atIndex(trcbuf_.get(0)->size()-1) );
    
    uiFunctionDisplay::Setup su;
    su.fillbelow(true).canvaswidth(600).canvasheight(400).drawborder(true)
      .drawliney2(true).editable(true).fillbelow(false).fillbelowy2(true)
      .curvzvaly(5).curvzvaly2(0).drawscattery1(true).yrg(scalerg)
      .xrg(zrg).ycol(Color(255,0,0));

    funcdisp_ = new uiFunctionDisplay( this, su );
    funcdisp_->xAxis()->setName( "Z" );
    funcdisp_->yAxis(true)->setName( "RMS Amplitude" ); 
    funcdisp_->yAxis(false)->setName( "Scale Factor" ); 

    uiGroup* mandispgrp = new uiGroup( this );
    mandispgrp->attach( alignedBelow, funcdisp_ );

    rangefld_ = new uiGenInput( mandispgrp, "Scale Range",
	    			FloatInpIntervalSpec() );
    rangefld_->valuechanged.notify( mCB(this,uiGainAnalysisDlg,dispRangeChgd ));
    rangefld_->setValue( scalerg );

    stepfld_ = new uiLabeledSpinBox( mandispgrp, "Gridline step");
    stepfld_->attach( rightOf, rangefld_ );
    stepfld_->box()->valueChanging.notify(
	    mCB(this,uiGainAnalysisDlg,dispRangeChgd) );
    if ( al.sd_.step<1 )
	stepfld_->box()->setNrDecimals( 2 );
    stepfld_->box()->setValue( al.sd_.step );

    ampscaletypefld_ = new uiGenInput( mandispgrp, "Amplitude Scale",
	    			    BoolInpSpec(true,"Linear","dB") );
    ampscaletypefld_->attach( rightTo, stepfld_ );
    ampscaletypefld_->valuechanged.notify(
	    mCB(this,uiGainAnalysisDlg,amplScaleTypeChanged) );

    setData( true );
}


uiGainAnalysisDlg::~uiGainAnalysisDlg()
{
}


void uiGainAnalysisDlg::convertZTo( bool milisec )
{
    const float factor = milisec ? 1000.00 : 1.0/1000.00;
    if ( zvals_.size() && SI().zIsTime() )
    {
	for ( int idx=0; idx<zvals_.size(); idx++ )
	    zvals_[idx] *= factor;
    }
}


void uiGainAnalysisDlg::setData( bool sety )
{
    TypeSet<float> zvals;
    TypeSet<float> scalefactors;
    
    SamplingData<float> zsd = trcbuf_.get(0)->info().sampling;
    StepInterval<float> zrg( zsd.start, zsd.atIndex(trcbuf_.get(0)->size()-1),
	    		     zsd.step );

    const TypeSet<float> yvals = funcdisp_->yVals();
    if ( !yvals.size() && !scalefactors_.size() )
    {
	Interval<float> scalerg = funcdisp_->yAxis(false)->range();
	scalefactors += scalerg.start;
	scalefactors += scalerg.start;
	zvals.erase();
	zvals += zrg.start;
	zvals += zrg.stop;
    }
    else
    {
	const TypeSet<float>& xvals = funcdisp_->xVals();
	zvals = scalefactors_.size() ? zvals_ : xvals;
	scalefactors = scalefactors_.size() ? scalefactors_ : yvals;
    }

    bool linear = ampscaletypefld_->getBoolValue();
    BufferString label( "RMS Amplitude" );
    label += linear ? "(Linear)" : "(dB)";

    funcdisp_->yAxis(true)->setName( label ); 
    
    TypeSet<float> avgrmsvals;

    const int nrsamples = trcbuf_.get(0)->size();
    for ( int sampnr=0; sampnr<nrsamples; sampnr++ )
    {
	float avgval = 0.0;
	int nrdefsamples = 0;
	for ( int trcnr=0; trcnr<trcbuf_.size(); trcnr++ )
	{
	    const SeisTrc* seisttrc = trcbuf_.get( trcnr );
	    float val = seisttrc->get( sampnr, 0 );
	    if ( !mIsUdf(val) )
	    {
		val = linear ? val : Math::toDB( val );
		if ( mIsUdf(val) )
		    continue;
		avgval += val;
		nrdefsamples++;
	    }
	}

	avgrmsvals += !avgval ? 0.0 : avgval/(float)nrdefsamples;
    }

    StepInterval<float> scalerg = rangefld_->getFInterval();
    scalerg.step = stepfld_->box()->getFValue();
    funcdisp_->setup().yrg_ = scalerg;

    if ( sety )
	funcdisp_->setVals( zvals.arr(), scalefactors.arr(), zvals.size() );

    funcdisp_->setY2Vals( zrg, avgrmsvals.arr(), zrg.nrSteps() );
    
    dispRangeChgd( 0 );
}


void uiGainAnalysisDlg::amplScaleTypeChanged( CallBacker* )
{
    setData();
}


void uiGainAnalysisDlg::dispRangeChgd( CallBacker* )
{
    StepInterval<float> range = rangefld_->getFInterval();
    if ( range.width() < stepfld_->box()->getFValue() )
    {
	rangefld_->setValue( funcdisp_->yAxis(false)->range() );
	return uiMSG().error( "Range step greater than range itself" );
    }

    range.step = stepfld_->box()->getFValue();
    const TypeSet<float>& yvals = funcdisp_->yVals();
    Interval<float> yvalrange( mUdf(float), -mUdf(float) );
    for ( int idx=0; idx<yvals.size(); idx++ )
	yvalrange.include( yvals[idx], false );

    if ( (!mIsUdf(yvalrange.start) && !range.includes(yvalrange.start,true)) ||
	 (!mIsUdf(-yvalrange.stop) && !range.includes(yvalrange.stop,true)) )
    {
	rangefld_->setValue( funcdisp_->yAxis(false)->range() );
	return uiMSG().error( "Scale Curve does not fit in the range" );
    }

    funcdisp_->yAxis(false)->setRange( range );
    funcdisp_->draw();
}


bool uiGainAnalysisDlg::acceptOK( CallBacker* )
{
    scalefactors_ = funcdisp_->yVals();
    zvals_ = funcdisp_->xVals();
    convertZTo( true );
    return true;
}


bool uiGainAnalysisDlg::rejectOK( CallBacker* )
{
    convertZTo( true );
    return true;
}
