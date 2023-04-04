/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigainanalysisdlg.h"

#include "uiaxishandlerbase.h"
#include "uifuncdispbase.h"
#include "uifunctiondisplayserver.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uispinbox.h"

#include "axislayout.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "survinfo.h"

uiGainAnalysisDlg::uiGainAnalysisDlg( uiParent* p, const SeisTrcBuf& traces,
      TypeSet<float>& zvals, TypeSet<float>& scalefac )
    : uiDialog(p,uiDialog::Setup(tr("Analyze Gain"), mNoDlgTitle, mNoHelpKey))
    , zvals_(zvals)
    , scalefactors_(scalefac)
    , trcbuf_(traces)
{
    convertZtoDisplay();

    Interval<float> scalerg( mUdf(float), -mUdf(float) );
    for ( int idx=0; idx<scalefac.size(); idx++ )
	scalerg.include( scalefac[idx], false );

    if ( !scalefac.size() )
	scalerg = Interval<float>(0,100);

    AxisLayout<float> al( scalerg );
    scalerg = Interval<float>( al.sd_.start, al.stop_ );

    SamplingData<float> zsd = trcbuf_.get(0)->info().sampling;
    Interval<float> zrg( zsd.start, zsd.atIndex(trcbuf_.get(0)->size()-1) );
    zrg.scale( SI().zDomain().userFactor() );

    uiFuncDispBase::Setup fdsu;
    fdsu.canvaswidth(600).canvasheight(400).drawborder(true)
      .drawliney2(true).editable(true).fillbelow(false).fillbelowy2(true)
      .curvzvaly(5).curvzvaly2(0).drawscattery1(true).yrg(scalerg)
      .xrg(zrg).ycol(OD::Color(255,0,0));

    funcdisp_ = GetFunctionDisplayServer().createFunctionDisplay( this, fdsu );
    funcdisp_->xAxis()->setCaption( SI().zDomain().getLabel() );
    funcdisp_->yAxis(true)->setCaption( tr("RMS Amplitude") );
    funcdisp_->yAxis(false)->setCaption( tr("Scale Factor") );

    uiGroup* mandispgrp = new uiGroup( this );
    mandispgrp->attach( alignedBelow, funcdisp_->uiobj() );

    rangefld_ = new uiGenInput( mandispgrp, tr("Scale Range"),
	    			FloatInpIntervalSpec() );
    rangefld_->valueChanged.notify( mCB(this,uiGainAnalysisDlg,dispRangeChgd ));
    rangefld_->setValue( scalerg );

    stepfld_ = new uiLabeledSpinBox( mandispgrp, tr("Gridline step"));
    stepfld_->attach( rightOf, rangefld_ );
    stepfld_->box()->valueChanging.notify(
	    mCB(this,uiGainAnalysisDlg,dispRangeChgd) );
    if ( al.sd_.step<1 )
	stepfld_->box()->setNrDecimals( 2 );
    stepfld_->box()->setValue( al.sd_.step );

    ampscaletypefld_ = new uiGenInput( mandispgrp, tr("Amplitude Scale"),
	    			    BoolInpSpec(true,tr("Linear"),tr("dB")) );
    ampscaletypefld_->attach( rightTo, stepfld_ );
    ampscaletypefld_->valueChanged.notify(
	    mCB(this,uiGainAnalysisDlg,amplScaleTypeChanged) );

    setData( true );
}


uiGainAnalysisDlg::~uiGainAnalysisDlg()
{
}


void uiGainAnalysisDlg::convertZtoDisplay()
{
    const float factor = SI().zDomain().userFactor();
    if ( zvals_.size() )
    {
	for ( int idx=0; idx<zvals_.size(); idx++ )
	    zvals_[idx] *= factor;
    }
}


void uiGainAnalysisDlg::convertZfromDisplay()
{
    const float factor = 1.f / SI().zDomain().userFactor();
    if ( zvals_.size() )
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
    zrg.scale( SI().zDomain().userFactor() );

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
    uiString label = toUiString("%1 %2 %3").arg(uiStrings::sRMS())
		     .arg(uiStrings::sAmplitude())
		     .arg(linear ? tr("(Linear)") : tr("(dB)"));

    funcdisp_->yAxis(true)->setCaption( label );

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

	avgrmsvals += !avgval ? 0.0f : avgval/(float)nrdefsamples;
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
	uiMSG().error( tr("Range step greater than range itself") );
	return;
    }

    range.step = stepfld_->box()->getFValue();
    const TypeSet<float>& yvals = funcdisp_->yVals();
    Interval<float> yvalrange( mUdf(float), -mUdf(float) );
    for ( int idx=0; idx<yvals.size(); idx++ )
    {
	if ( mIsUdf(yvals[idx]) )
	    continue;
	yvalrange.include( yvals[idx], false );
    }

    if ( (!mIsUdf(yvalrange.start) && !range.includes(yvalrange.start,true)) ||
	 (!mIsUdf(-yvalrange.stop) && !range.includes(yvalrange.stop,true)) )
    {
	rangefld_->setValue( funcdisp_->yAxis(false)->range() );
	uiMSG().error( tr("Scale Curve does not fit in the range") );
	return;
    }

    funcdisp_->yAxis(false)->setRange( range );
    funcdisp_->draw();
}


bool uiGainAnalysisDlg::acceptOK( CallBacker* )
{
    scalefactors_ = funcdisp_->yVals();
    while ( scalefactors_.isPresent(mUdf(float)) )
	scalefactors_ -= mUdf(float);
    zvals_ = funcdisp_->xVals();
    convertZfromDisplay();
    return true;
}


bool uiGainAnalysisDlg::rejectOK( CallBacker* )
{
    convertZfromDisplay();
    return true;
}
