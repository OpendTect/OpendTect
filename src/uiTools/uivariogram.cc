/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivariogram.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "bufstringset.h"
#include "datapointset.h"
#include "executor.h"
#include "interpol1d.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "statrand.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "variogramcomputers.h"

#include "uiaxishandler.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitaskrunner.h"
#include "variogrammodels.h"
#include "od_helpids.h"

#include <math.h>

uiVariogramDlg::uiVariogramDlg( uiParent* p, bool isvert )
    : uiDialog(p,uiDialog::Setup(tr("Semi-variogram parameters"),
				 tr("Specify semi-variogram parameters"),
				 mODHelpKey(mVariogramDlgHelpID) ) )
{
    const int dxmin = SI().inlDistance() <= SI().crlDistance() ?
		(int)SI().inlDistance() : (int)SI().crlDistance();
    const int minrgval = isvert ? 30 : SI().xyInFeet() ?
	dxmin*mNINT32((float)300/dxmin) : dxmin*mNINT32((float) 100/dxmin);
    const int maxrgval = isvert ? 300 : SI().xyInFeet() ?
	dxmin*mNINT32((float) 10000/dxmin) : dxmin*mNINT32((float) 5000/dxmin);
    const int defrgval = isvert ? 50 : SI().xyInFeet() ?
	dxmin*mNINT32((float) 5000/dxmin) : dxmin*mNINT32((float) 2000/dxmin);
    const int minstepval = isvert ? 1 : dxmin;
    const int maxstepval = isvert ? 10 : 10*dxmin;
    const int defstep = isvert ? 1 : dxmin;
    const int minfldval = isvert ? 1 : 1000;
    const int maxfldval = isvert ? 100 : 100000;
    const int deffldval = isvert ? 10 : 1000;
    const int deffldstep = isvert ? 1 : 1000;

    uiString unitstr =
		isvert ? SI().getUiZUnitString() : SI().getUiXYUnitString();
    uiString lbl( tr("Maximum range %1").arg(unitstr) );
    uiLabeledSpinBox* lblmaxrgfld = new uiLabeledSpinBox( this, lbl, 0 );
    maxrgfld_ = lblmaxrgfld->box();
    maxrgfld_->doSnap( true );
    maxrgfld_->setInterval( minrgval, maxrgval, defstep );
    maxrgfld_->setValue( defrgval );

    uiString lbl2(
	uiStrings::phrJoinStrings(uiStrings::sStep(),tr("%1").arg(unitstr)) );
    uiLabeledSpinBox* lblstepfld = new uiLabeledSpinBox( this, lbl2, 0 );
    stepfld_ = lblstepfld->box();
    stepfld_->setInterval( minstepval, maxstepval, defstep );
    stepfld_->setValue( defstep );
    stepfld_->valueChanged.notify(mCB(this,uiVariogramDlg,stepChgCB));
    lblstepfld->attach( alignedBelow, lblmaxrgfld );
    lblstepfld->display( isvert );

    uiString lbl3 = tr("%1 number of pairs per lag distance")
		    .arg(isvert ? tr("Min") : tr("Max"));
    uiLabeledSpinBox* lblfoldfld = new uiLabeledSpinBox( this, lbl3, 0 );
    foldfld_ = lblfoldfld->box();
    foldfld_->setInterval( minfldval, maxfldval, deffldstep );
    foldfld_->setValue( deffldval );
    lblfoldfld->attach( alignedBelow, lblstepfld );

    stepChgCB(0);
}


uiVariogramDlg::~uiVariogramDlg()
{}


void uiVariogramDlg::stepChgCB( CallBacker* )
{
    const int val = stepfld_->getIntValue();
    maxrgfld_->setStep( val, true );
}


int uiVariogramDlg::getMaxRg() const
{
    return maxrgfld_->getIntValue();
}


int uiVariogramDlg::getStep() const
{
    return stepfld_->getIntValue();
}


int uiVariogramDlg::getFold() const
{
    return foldfld_->getIntValue();
}

//------------------------------------------------------------------------

static const char* typestrs[] =
{
    "exponential",
    "spherical",
    "gaussian",
    0
};

uiVariogramDisplay::uiVariogramDisplay ( uiParent* p, Array2D<float>* data,
					 Array2D<float>* axes,
					 BufferStringSet* labels,
					 int maxrg, bool ishor )
	: uiDialog(p,uiDialog::Setup(tr("Variogram analysis"),
				     tr("Variogram analysis"),
				     mODHelpKey(mVariogramDisplayHelpID) )
                                     .modal(false))
	, maxrg_(maxrg)
{
    if ( !data || ! axes || !labels ) return;

    data_ = new Array2DImpl<float>( *data );
    axes_ = new Array2DImpl<float>( *axes );
    labels_ = new BufferStringSet( *labels);
    setCtrlStyle( CloseOnly );
    const CallBack chgCBfld ( mCB(this,uiVariogramDisplay,fieldChangedCB) );
    const CallBack chgCBlbl ( mCB(this,uiVariogramDisplay,labelChangedCB) );
    sillfld_ = new uiSlider( this, uiSlider::Setup(tr("sill"))
					.withedit(true).nrdec(3).logscale(false)
					.isvertical(true), "sill slider" );
    sillfld_->display( true );
    sillfld_->valueChanged.notify( chgCBfld );

    uiFunctionDisplay::Setup fdsu;
    fdsu.border_.setLeft( 2 );
    fdsu.border_.setRight( 2 );
    fdsu.drawscattery1( true );
    fdsu.drawliney( true );
    fdsu.noy2axis( true );
    fdsu.useyscalefory2( true );
    disp_ = new uiFunctionDisplay( this, fdsu );
    const uiString unitstr =
		ishor ? SI().getUiXYUnitString() : SI().getUiZUnitString();
    uiString xnmstr = tr("Lag distance %1").arg(unitstr);
    disp_->xAxis()->setCaption( xnmstr );
    disp_->yAxis(false)->setCaption( tr("Normalized Variance") );
    disp_->attach( rightOf, sillfld_ );

    rangefld_ = new uiSlider( this, uiSlider::Setup(tr("range")).withedit(true).
						    nrdec(1).logscale(false),
				   "range slider" );
    rangefld_->attach( centeredBelow, disp_ );
    rangefld_->setMinValue( 0 );
    rangefld_->display( true );
    rangefld_->valueChanged.notify( chgCBfld );

    typefld_ = new uiGenInput( this, tr("Variogram model"),
			     StringListInpSpec(typestrs) );
    typefld_->valueChanged.notify( chgCBfld );
    typefld_->attach( alignedBelow, rangefld_ );

    uiString lblnmstr = ishor ? tr("Direction:") : tr("Source:");
    labelfld_ = new uiGenInput( this, lblnmstr,
				StringListInpSpec(*labels) );
    labelfld_->valueChanged.notify( chgCBlbl );
    labelfld_->attach( rightAlignedAbove, disp_ );
}


void uiVariogramDisplay::draw()
{
    float maxdataval = 0;
    float maxdatavalcomp1 = 0;
    const int nrcomp = data_->info().getSize(0);
    const int size = data_->info().getSize(1);
    for ( int icomp=0; icomp<nrcomp; icomp++ )
    {
	for ( int ilag=0; ilag<size; ilag++ )
	{
	    float tmpval = data_->get(icomp,ilag);
	    if ( tmpval>maxdataval && !mIsUdf(tmpval) )
		maxdataval = tmpval;
	    if ( icomp == 0 && ilag == size-1 )
		maxdatavalcomp1 = maxdataval;
	}
    }

    labelChangedCB(0);
    disp_->setup().xrg_.stop = mCast( float, maxrg_ );
    disp_->setup().yrg_.stop = maxdataval*1.1f;

    rangefld_->setMaxValue( mCast( float, maxrg_ ) );
    rangefld_->setStep( mCast( float, maxrg_/(100*(size-1)) ) );
    rangefld_->setValue( maxrg_/4 );

    sillfld_->setMinValue( 0 );
    sillfld_->setMaxValue( maxdataval*1.1f );
    sillfld_->setStep( maxdataval/1000 );
    sillfld_->setValue( maxdatavalcomp1 );

    fieldChangedCB(0);
}


void uiVariogramDisplay::labelChangedCB( CallBacker* c )
{
    const int size = axes_->info().getSize(1);
    const int curcomp = labelfld_->getIntValue();
    int nbpts = 0;
    TypeSet<float> xaxisvals;
    TypeSet<float> yaxisvals;
    for ( int ilag=0; ilag<size; ilag++ )
    {
	if ( !mIsUdf(data_->get(curcomp,ilag)) )
	{
	    xaxisvals += axes_->get(curcomp,ilag);
	    yaxisvals += data_->get(curcomp,ilag);
	    nbpts++;
	}
    }
    disp_->setVals( xaxisvals.arr(), yaxisvals.arr(), nbpts );
}


void uiVariogramDisplay::fieldChangedCB( CallBacker* c )
{
    const float nugget = 0;
    const int size = axes_->info().getSize(1);
    Array1DImpl<float> xaxisvals(size);
    Array1DImpl<float> yaxisvals(size);

    const int curcomp = labelfld_->getIntValue();
    const float sill = sillfld_->getFValue();
    const float range = rangefld_->getFValue();

    for ( int ilag=0; ilag<size; ilag++ )
	xaxisvals.set( ilag, axes_->get(curcomp,ilag) );
    getVariogramModel( typefld_->text(), nugget, sill, range, size,
			xaxisvals.getData(), yaxisvals.getData() );

    disp_->setY2Vals( xaxisvals.getData(), yaxisvals.getData(), size );
}


uiVariogramDisplay::~uiVariogramDisplay()
{
    if ( data_ ) delete data_;
    if ( axes_ ) delete axes_;
    if ( labels_ ) delete labels_;
}
