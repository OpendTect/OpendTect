/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck & H. Huck
 Date:		Sep 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id Exp $";

#include "uivariogram.h"

#include "arrayndimpl.h"
#include "datapointset.h"
#include "statrand.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "uiaxishandler.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uislider.h"
#include "uispinbox.h"
#include "variogrammodels.h" 

#include <math.h>

uiVariogramDlg::uiVariogramDlg( uiParent* p, bool isvert )
    : uiDialog(p,uiDialog::Setup("Semi-variogram parameters",
				 "Specify semi-variogram parameters",
				 mTODOHelpID ) )
{
    int minrgval = isvert ? 40 : SI().xyInFeet() ? 300 : 100;
    int maxrgval = isvert ? 100 : SI().xyInFeet() ? 5000 : 2000;
    int defrgval = isvert ? 50 : SI().xyInFeet() ? 3000 : 1000;
    int minstepval = isvert ? 1 : SI().xyInFeet() ? 50 : 25;
    int defstep = isvert ? 1 : SI().xyInFeet() ? 50 : 25;
    int minfldval = isvert ? 2000 : 1000;
    int maxfldval = isvert ? 20000 : 100000;
    int deffldstep = isvert ? 500 : 1000;
    int deffldval = isvert ? 2000 : 1000;
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
    stepfld_->setInterval( minstepval, minrgval, minstepval );
    stepfld_->setValue( defstep );
    stepfld_->valueChanged.notify(mCB(this,uiVariogramDlg,stepChgCB));
    lblstepfld->attach( alignedBelow, lblmaxrgfld );

    BufferString lbl3( "Max number of pairs per lag distance" );
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

//-------------------------------------------------------------------------

HorVariogramComputer::HorVariogramComputer( DataPointSet& dpset, int step,
					    int range, int fold )
	: variogramvals_( new Array1DImpl<float>(range/step+1) )
{
	compVarFromRange( dpset, step, range, fold );
}


HorVariogramComputer::~HorVariogramComputer()
{
    delete variogramvals_;
}


void HorVariogramComputer::compVarFromRange( DataPointSet& dpset,
					     int step, int range, int fold )
{
    Stats::RunCalcSetup rcsetuptot;
    rcsetuptot.require( Stats::Variance );
    Stats::RunCalc<float> statstot( rcsetuptot );
    for ( DataPointSet::RowID irow=0; irow<dpset.size(); irow++ )
	statstot+=dpset.getValues( irow )[1];

    float totvar = statstot.variance();

    variogramvals_->set( 0, 0 );
    StepInterval<int> inlrg = dpset.bivSet().inlRange();
    StepInterval<int> crlrg = dpset.bivSet().crlRange();
    for( int lag=step; lag<=range; lag+=step )
    {
	int mininl = inlrg.start+int(lag/SI().inlDistance())+1;
	int maxinl = inlrg.stop-int(lag/SI().inlDistance())-1;
	int mincrl = crlrg.start+int(lag/SI().crlDistance())+1;
	int maxcrl = crlrg.stop-int(lag/SI().crlDistance())-1;
	Array2DImpl<float> data(fold,3);

	Stats::RunCalcSetup rcsetup;
	rcsetup.require( Stats::Average );
	Stats::RunCalc<float> stats( rcsetup );

	for( int ifold=0; ifold<fold; ifold++ )
	{
	    int posinl1 = mininl + mNINT((maxinl-mininl)*Stats::RandGen::get());
	    int poscrl1 = mincrl + mNINT((maxcrl-mincrl)*Stats::RandGen::get());
	    Coord pos1 = SI().transform(BinID(posinl1,poscrl1));
	    DataPointSet::RowID posval1 =
			dpset.findFirst(BinID(posinl1,poscrl1));
	    if ( posval1<0 ) continue;

	    float azi = Stats::RandGen::get()*2*M_PI;
	    Coord pos2 = Coord(pos1.x+lag*sin(azi),pos1.y+lag*cos(azi));
	    BinID bidpos2 = SI().transform(pos2);
	    DataPointSet::RowID posval2 = dpset.findFirst(bidpos2);
	    if ( posval2<0 ) continue;

	    //remark: we cannot use the functions of the DataPointSet here
	    //because it is created in a very wrong way: creation and then
	    //modification of the BinIDValueSet : the DataPointSet does not
	    //know about the columns!
	    //thus we use a workaround: getValues 0=shift, 1=surface data, 2=z
	    //if this order would change we are in trouble-> even lead to crash.
	    float val1 = dpset.getValues( posval1 )[1];
	    float val2 = dpset.getValues( posval2 )[1];

	    if ( mIsUdf(val1) || mIsUdf(val2) || mIsZero(val1-val2,1e-6) )
	       	continue;

	    Coord usedpos2 = SI().transform(bidpos2);
	    float dist = sqrt( (usedpos2.x - pos1.x)*(usedpos2.x - pos1.x)
		    	     + (usedpos2.y - pos1.y)*(usedpos2.y - pos1.y) );
	    float diffval = 0.5*(val2-val1)*(val2-val1);
	    data.set( ifold, 0, diffval );
	    data.set( ifold, 1, dist );
	    data.set( ifold, 2, azi );
	    stats += diffval;
	}

	variogramvals_->set( lag/step, stats.average()/totvar );
    }
}

Array1D<float>* HorVariogramComputer::getData() const
{
    return variogramvals_;
}


//-------------------------------------------------------------------------

VertVariogramComputer::VertVariogramComputer( DataPointSet& dpset, int colid,
					      int step, int range, int fold )
	: variogramvals_( new Array1DImpl<float>(range/step+1) )
{
	compVarFromRange( dpset, colid, step, range, fold );
}


VertVariogramComputer::~VertVariogramComputer()
{
    delete variogramvals_;
}


void VertVariogramComputer::compVarFromRange( DataPointSet& dpset, int colid,
					      int step, int range, int fold )
{
    DataPointSet::ColID dpcolid(colid);
    Stats::RunCalcSetup rcsetuptot;
    rcsetuptot.require( Stats::Variance );
    Stats::RunCalc<float> statstot( rcsetuptot );
    for ( DataPointSet::RowID irow=0; irow<dpset.size(); irow++ )
	statstot+=dpset.value( dpcolid, irow );

    float totvar = statstot.variance();

    variogramvals_->set( 0, 0 );
    for( int lag=step; lag<=range; lag+=step )
    {
	Array2DImpl<float> data(fold,2);

	Stats::RunCalcSetup rcsetup;
	rcsetup.require( Stats::Average );
	Stats::RunCalc<float> stats( rcsetup );

	for( int ifold=0; ifold<fold; ifold++ )
	{
	    int rdmpos1 = mNINT( dpset.size()*Stats::RandGen::get() );
	    if ( rdmpos1 == dpset.size() ) rdmpos1 -=1;

	    DataPointSet::RowID rowpos1(rdmpos1);
	    int grppos1 = dpset.group(rowpos1);
	    float zpos1 = dpset.z(rowpos1);
	    float val1 = dpset.value( dpcolid, rowpos1 );
	    DataPointSet::Pos pospos1 = dpset.pos( rowpos1 );
	    int rowpos2 = 0;
	    float dz = 0;
	    for( int irow=rdmpos1-1; irow>=0; irow-- )
	    {
		DataPointSet::RowID irowpos2(irow); 
		int igrppos2 = dpset.group(irowpos2);
		if ( igrppos2 != grppos1 ) continue;
		float zpos2 = dpset.z(irowpos2);
		dz = zpos1 - zpos2;
		if ( dz > ((float)lag/1000) )
		{
		    rowpos2 = irow;
		    break;
		}
	    }
	    if ( rowpos2<0 ) continue;

	    int grppos2 = dpset.group(rowpos2);
	    if ( grppos2 != grppos1 ) continue;

	    float val2 = dpset.value( dpcolid, rowpos2 );

	    if ( mIsUdf(val1) || mIsUdf(val2) || mIsZero(val1-val2,1e-6) )
	       	continue;
	    float diffval = 0.5*(val2-val1)*(val2-val1);
	    data.set( ifold, 0, diffval );
	    data.set( ifold, 1, dz );
	    stats += diffval;
	}

	variogramvals_->set( lag/step, stats.average()/totvar );
    }
}

Array1D<float>* VertVariogramComputer::getData() const
{
    return variogramvals_;
}


//------------------------------------------------------------------------

static const char* typestrs[] =
{
    "exponential",
    "spherical",
    "gaussian",
    0
};

uiVariogramDisplay::uiVariogramDisplay ( uiParent* p, Array1D<float>* data,
					 int maxrg, int step, bool ishor )
    	: uiDialog(p,uiDialog::Setup("Variogram analysis","Variogram analysis",
		    		     mTODOHelpID ))
	, data_(data)
	, maxrg_(maxrg)
	, step_(step)
{
    const CallBack chgCB ( mCB(this,uiVariogramDisplay,fieldChangedCB) );
    sillfld_ = new uiSliderExtra( this,                              
				  uiSliderExtra::Setup("sill").withedit(true).
						       nrdec(3).logscale(false).						       isvertical(true),
				  "sill slider" ); 
    sillfld_->display( true );
    sillfld_->sldr()->valueChanged.notify( chgCB );

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
    xnmstr += ishor ? "(ms)" : SI().getXYUnitString();
    disp_->xAxis()->setName( xnmstr.buf() );
    disp_->yAxis(false)->setName( "Normalized Variance" );
    disp_->attach( rightOf, sillfld_ );

    rangefld_ = new uiSliderExtra( this,                              
				   uiSliderExtra::Setup("range").withedit(true).
						       nrdec(1).logscale(false),
				   "range slider" ); 
    rangefld_->attach( alignedBelow, disp_ );
    rangefld_->sldr()->setMinValue( 0 );
    rangefld_->display( true );
    rangefld_->sldr()->valueChanged.notify( chgCB );

    typefld_ = new uiGenInput( this, "Variogram model",
	    		     StringListInpSpec(typestrs) );
    typefld_->valuechanged.notify( chgCB );
    typefld_->attach( alignedBelow, rangefld_ );
}

void uiVariogramDisplay::draw()
{
    TypeSet<float> xaxisvals;
    float maxdataval = 0;
    for ( int ilag=0; ilag <maxrg_; ilag+=step_ )
    {
	xaxisvals += ilag;
	float tmpval = data_->get(ilag/step_);
	if ( tmpval>maxdataval )
	    maxdataval = tmpval;
    }

    int size = xaxisvals.size();
    disp_->setVals( xaxisvals.arr(), data_->getData(), size );

    rangefld_->sldr()->setMaxValue( maxrg_ );
    rangefld_->sldr()->setStep( step_/100 );
    rangefld_->sldr()->setValue( maxrg_/4 );

    sillfld_->sldr()->setMinValue( 0 );
    sillfld_->sldr()->setMaxValue( maxdataval );
    sillfld_->sldr()->setStep( maxdataval/1000 );
    sillfld_->sldr()->setValue(data_->get(size-1) );

    fieldChangedCB(0);
}


void uiVariogramDisplay::fieldChangedCB( CallBacker* c )
{
    float nugget = 0;
    TypeSet<float> xaxisvals;
    for ( int ilag=0; ilag <maxrg_; ilag+=step_ )
	xaxisvals += ilag;

    int size = xaxisvals.size();
    float sill = sillfld_->sldr()->getValue();
    float range = rangefld_->sldr()->getValue();
    Array1DImpl<float> out(size);
    getVariogramModel( typefld_->text(), nugget, sill, range, size,
	   		xaxisvals.arr(), out.getData() );
    disp_->setY2Vals( xaxisvals.arr(), out.getData(), size );
}


