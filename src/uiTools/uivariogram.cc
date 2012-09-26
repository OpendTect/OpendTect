/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck & H. Huck
 Date:		Sep 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id Exp $";

#include "uivariogram.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
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

#include <math.h>

uiVariogramDlg::uiVariogramDlg( uiParent* p, bool isvert )
    : uiDialog(p,uiDialog::Setup("Semi-variogram parameters",
				 "Specify semi-variogram parameters",
				 "111.0.12" ) )
{
    int dxmin = SI().inlDistance() <= SI().crlDistance() ?
       		(int)SI().inlDistance() : (int)SI().crlDistance();
    int minrgval = isvert ? 30 : SI().xyInFeet() ?
       			    dxmin*mNINT32((float)300/dxmin) : dxmin*mNINT32((float) 100/dxmin);
    int maxrgval = isvert ? 300 : SI().xyInFeet() ? 
			    dxmin*mNINT32((float) 10000/dxmin) : dxmin*mNINT32((float) 5000/dxmin);
    int defrgval = isvert ? 50 : SI().xyInFeet() ?
       			    dxmin*mNINT32((float) 5000/dxmin) : dxmin*mNINT32((float) 2000/dxmin);
    int minstepval = isvert ? 1 : dxmin;
    int maxstepval = isvert ? 10 : 10*dxmin;
    int defstep = isvert ? 1 : dxmin;
    int minfldval = isvert ? 1 : 1000;
    int maxfldval = isvert ? 100 : 100000;
    int deffldval = isvert ? 10 : 1000;
    int deffldstep = isvert ? 1 : 1000;

    BufferString lbl( "Maximum range " );
    lbl += isvert ? "(ms)" : SI().getXYUnitString();
    uiLabeledSpinBox* lblmaxrgfld = new uiLabeledSpinBox( this, lbl, 0 );
    maxrgfld_ = lblmaxrgfld->box();
    maxrgfld_->doSnap( true );
    maxrgfld_->setInterval( minrgval, maxrgval, defstep );
    maxrgfld_->setValue( defrgval );

    BufferString lbl2( "Step " );
    lbl2 += isvert ? "(ms)" : SI().getXYUnitString();
    uiLabeledSpinBox* lblstepfld = new uiLabeledSpinBox( this, lbl2, 0 );
    stepfld_ = lblstepfld->box();
    stepfld_->setInterval( minstepval, maxstepval, defstep );
    stepfld_->setValue( defstep );
    stepfld_->valueChanged.notify(mCB(this,uiVariogramDlg,stepChgCB));
    lblstepfld->attach( alignedBelow, lblmaxrgfld );
    lblstepfld->display( isvert );

    BufferString lbl3 = isvert ? "Min " : "Max ";
    lbl3 += "number of pairs per lag distance";
    uiLabeledSpinBox* lblfoldfld = new uiLabeledSpinBox( this, lbl3, 0 );
    foldfld_ = lblfoldfld->box();
    foldfld_->setInterval( minfldval, maxfldval, deffldstep );
    foldfld_->setValue( deffldval );
    lblfoldfld->attach( alignedBelow, lblstepfld );

    stepChgCB(0);
}


void uiVariogramDlg::stepChgCB( CallBacker* )
{
    int val = stepfld_->getValue();
    maxrgfld_->setStep( val, true );
}


int uiVariogramDlg::getMaxRg() const
{
    return maxrgfld_->getValue();
}


int uiVariogramDlg::getStep() const
{
    return stepfld_->getValue();
}


int uiVariogramDlg::getFold() const
{
    return foldfld_->getValue();
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
    	: uiDialog(p,uiDialog::Setup("Variogram analysis","Variogram analysis",
		    		     "111.0.13" ).modal(false))
	, maxrg_(maxrg)
{
    if ( !data || ! axes || !labels ) return;

    data_ = new Array2DImpl<float>( *data );
    axes_ = new Array2DImpl<float>( *axes );
    labels_ = new BufferStringSet( *labels);
    setCtrlStyle( LeaveOnly );
    const CallBack chgCBfld ( mCB(this,uiVariogramDisplay,fieldChangedCB) );
    const CallBack chgCBlbl ( mCB(this,uiVariogramDisplay,labelChangedCB) );
    sillfld_ = new uiSliderExtra( this,                              
				  uiSliderExtra::Setup("sill").withedit(true).
						       nrdec(3).logscale(false).						       isvertical(true),
				  "sill slider" ); 
    sillfld_->display( true );
    sillfld_->sldr()->valueChanged.notify( chgCBfld );

    uiFunctionDisplay::Setup fdsu;
    fdsu.border_.setLeft( 2 );
    fdsu.border_.setRight( 2 );
    fdsu.epsaroundzero_ = 1e-3;
    fdsu.drawscattery1( true );
    fdsu.drawliney( true );
    fdsu.noy2axis( true );
    fdsu.useyscalefory2( true );
    disp_ = new uiFunctionDisplay( this, fdsu );
    BufferString xnmstr = "Lag distance ";
    xnmstr += ishor ? SI().getXYUnitString() : "(ms)";
    disp_->xAxis()->setName( xnmstr.buf() );
    disp_->yAxis(false)->setName( "Normalized Variance" );
    disp_->attach( rightOf, sillfld_ );

    rangefld_ = new uiSliderExtra( this,                              
				   uiSliderExtra::Setup("range").withedit(true).
						       nrdec(1).logscale(false),
				   "range slider" ); 
    rangefld_->attach( centeredBelow, disp_ );
    rangefld_->sldr()->setMinValue( 0 );
    rangefld_->display( true );
    rangefld_->sldr()->valueChanged.notify( chgCBfld );

    typefld_ = new uiGenInput( this, "Variogram model",
	    		     StringListInpSpec(typestrs) );
    typefld_->valuechanged.notify( chgCBfld );
    typefld_->attach( alignedBelow, rangefld_ );

    BufferString lblnmstr = ishor ? "Direction:" : "Source:";
    labelfld_ = new uiGenInput( this, lblnmstr,
	    			StringListInpSpec(*labels) );
    labelfld_->valuechanged.notify( chgCBlbl );
    labelfld_->attach( rightAlignedAbove, disp_ );
}

void uiVariogramDisplay::draw()
{
    float maxdataval = 0;
    float maxdatavalcomp1 = 0;
    int nrcomp = data_->info().getSize(0);
    int size = data_->info().getSize(1);
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
    disp_->setup().xrg_.stop = maxrg_;
    disp_->setup().yrg_.stop = maxdataval*1.1f;

    rangefld_->sldr()->setMaxValue( maxrg_ );
    rangefld_->sldr()->setStep( maxrg_/(100*(size-1)) );
    rangefld_->sldr()->setValue( maxrg_/4 );

    sillfld_->sldr()->setMinValue( 0 );
    sillfld_->sldr()->setMaxValue( maxdataval*1.1f );
    sillfld_->sldr()->setStep( maxdataval/1000 );
    sillfld_->sldr()->setValue( maxdatavalcomp1 );

    fieldChangedCB(0);
}

void uiVariogramDisplay::labelChangedCB( CallBacker* c )
{
    int size = axes_->info().getSize(1);
    int curcomp = labelfld_->getIntValue();
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
    float nugget = 0;
    int size = axes_->info().getSize(1);
    Array1DImpl<float> xaxisvals(size);
    Array1DImpl<float> yaxisvals(size);

    int curcomp = labelfld_->getIntValue();
    float sill = sillfld_->sldr()->getValue();
    float range = rangefld_->sldr()->getValue();

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
