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
#include "arrayndutils.h"
#include "bufstringset.h"
#include "datapointset.h"
#include "interpol1d.h"
#include "posvecdataset.h"
#include "statrand.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "uiaxishandler.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uimsg.h"
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
    int maxrgval = isvert ? 100 : SI().xyInFeet() ? 10000 : 5000;
    int defrgval = isvert ? 50 : SI().xyInFeet() ? 5000 : 2000;
    int minstepval = isvert ? 1 : SI().xyInFeet() ? 50 : 25;
    int defstep = isvert ? 1 : SI().xyInFeet() ? 50 : 25;
    int minfldval = isvert ? 1 : 1000;
    int maxfldval = isvert ? 100 : 100000;
    int deffldstep = isvert ? 1 : 1000;
    int deffldval = isvert ? 10 : 1000;
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

//-------------------------------------------------------------------------

HorVariogramComputer::HorVariogramComputer( DataPointSet& dpset, int step,
					    int range, int fold )
	: variogramvals_( new Array2DImpl<float>(3,range/step+1) )
	, variogramnms_( new BufferStringSet )
{
    dataisok_ = compVarFromRange( dpset, step, range, fold );
}


HorVariogramComputer::~HorVariogramComputer()
{
    delete variogramvals_;
    delete variogramnms_;
}


bool HorVariogramComputer::compVarFromRange( DataPointSet& dpset,
					     int step, int range, int fold )
{
    Stats::CalcSetup rcsetuptot;
    rcsetuptot.require( Stats::Variance );
    Stats::RunCalc<double> statstot( rcsetuptot );
    for ( DataPointSet::RowID irow=0; irow<dpset.size(); irow++ )
	statstot+=dpset.getValues( irow )[1];

    float totvar = (float)statstot.variance();

    if ( mIsZero( totvar , 1e-6 ) )
    {
	BufferString emsg = "Failed to compute the total variance\n";
	emsg += "Please check the input data";
	uiMSG().error( emsg.buf() );
	return false;
    }

    StepInterval<int> inlrg = dpset.bivSet().inlRange();
    StepInterval<int> crlrg = dpset.bivSet().crlRange();
    for( int lag=step; lag<=range; lag+=step )
    {
	int mininl = inlrg.start+int(lag/SI().inlDistance())+1;
	int maxinl = inlrg.stop-int(lag/SI().inlDistance())-1;
	int mincrl = crlrg.start+int(lag/SI().crlDistance())+1;
	int maxcrl = crlrg.stop-int(lag/SI().crlDistance())-1;
	Array2DImpl<float> data(fold,3);

	Stats::CalcSetup rcsetup;
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

	for ( int icomp = 0; icomp < 3 ; icomp++ )
	{
	    variogramvals_->set( icomp, 0, 0 );
	    variogramvals_->set( icomp, lag/step, stats.average()/totvar );
	}
    }

    variogramnms_->add("Inline");
    variogramnms_->add("Diagonal");
    variogramnms_->add("Crossline");

    return true;
}

Array2D<float>* HorVariogramComputer::getData() const
{
    return variogramvals_;
}

BufferStringSet* HorVariogramComputer::getLabels() const
{
    return variogramnms_;
}


//-------------------------------------------------------------------------

VertVariogramComputer::VertVariogramComputer( DataPointSet& dpset, int colid,
					      int step, int range, int fold,
       					      int nrgroups )
	: variogramvals_( new Array2DImpl<float>(nrgroups+1,range/step+1) )
	, variogramstds_( new Array2DImpl<float>(nrgroups+1,range/step) )
	, variogramfolds_( new Array2DImpl<od_int64>(nrgroups+1,range/step) )
	, variogramnms_( new BufferStringSet )
{
    dataisok_ = compVarFromRange( dpset, colid, step, range, fold, nrgroups );
}


VertVariogramComputer::~VertVariogramComputer()
{
    delete variogramvals_;
    delete variogramstds_;
    delete variogramfolds_;
    delete variogramnms_;
}


bool VertVariogramComputer::compVarFromRange( DataPointSet& dpset, int colid,
					      int step, int range, int fold,
       					      int nrgroups )
{
    DataPointSet::ColID dpcolid(colid);
    Stats::CalcSetup rcsetuptot;
    rcsetuptot.require( Stats::Variance );
    rcsetuptot.require( Stats::Count );
    Stats::RunCalc<double> statstot( rcsetuptot );
    int nrwells = 0;
    int nrcontribwells = 0;
    float zstep = SI().zIsTime() ? 1000 : 1;

    variogramvals_->set( 0, 0, 0 );
    variogramnms_->add("AllWells");

    BufferStringSet grpnames;
    dpset.dataSet().pars().get( "Groups", grpnames );

    if ( grpnames.size() == 0 )
    {
	BufferString emsg = "Could not retrieve group names.";
	uiMSG().error( emsg.buf() );
	return false;
    }

    for ( int igroup=1; igroup<=nrgroups; igroup++ )
    {
	TypeSet<MDandRowID> disorder;
	bool grouphasdata = false;

	for ( int irow=dpset.size()-1; irow>=0; irow-- )
	{
	    if ( dpset.group(irow) == igroup )
	    {
		grouphasdata = true;
		if ( !mIsUdf(dpset.value( dpcolid, irow )) )
		    disorder += MDandRowID( dpset.z(irow), irow );
	    }
	}
	sort( disorder );
	int nrin = disorder.size();
	if ( grouphasdata )
	{
	    nrwells++;
	    if ( nrin == 0 )
		continue;
	    else
		nrcontribwells++;
	}
	else
	    continue;

	for ( int idz = 0; idz<nrin-1; idz++ )
	{
	    if ( mIsZero( disorder[idz].md_ - disorder[idz+1].md_ , 1e-6 ) )
	    {
		BufferString emsg = "Data inappropriate for analysis.\n";
		emsg += "Please re-extract with Radius around wells = 0";
		uiMSG().error( emsg.buf() );
		return false;
	    }
	}
	int ztop  = mNINT(dpset.z(disorder[0].rowid_)*zstep)+step;
	int zbase = mNINT(dpset.z(disorder[nrin-1].rowid_)*zstep)-step;

	if ( zbase >  dpset.z(disorder[nrin-1].rowid_)*zstep ||
	     ztop  <  dpset.z(disorder[0].rowid_)*zstep || 
	     (zbase - ztop) < 3*step )
	{
	    BufferString emsg ="Z interval too small for analysis.\n";
	    emsg += "Well ";
	    emsg += grpnames.get( igroup-1 );
	    emsg += " is not used";
	    uiMSG().error( emsg.buf() );
	    continue;
	}

	int nrout = (zbase-ztop)/step+1;
	Array1DImpl<double> interpolatedvals(nrout);

	int previdx = 0;
	double depth_out = ztop/zstep;
	for ( int idz = 0; idz<nrout; idz++ )
	{
	    while ( dpset.z(disorder[previdx+1].rowid_) < depth_out )
	    {
		previdx++;
	    }
	    if ( previdx >= nrin )
	    {
		BufferString emsg ="Interpolation error.\n";
		emsg += "Well ";
		emsg += grpnames.get( igroup-1 );
		emsg += " is not used";
		uiMSG().error( emsg.buf() );
		continue;
	    }
	    float reldist = (depth_out-dpset.z(disorder[previdx].rowid_))/
		  (dpset.z(disorder[previdx+1].rowid_)-
		   dpset.z(disorder[previdx].rowid_));
	    double val_out = Interpolate::
			     linearReg1D(dpset.value( dpcolid,
					       disorder[previdx].rowid_ ),
				     	 dpset.value( dpcolid, 
					       disorder[previdx+1].rowid_ ),
		    			 reldist);
	    interpolatedvals.set(idz,val_out);
	    depth_out += (double)(step/zstep);
	}

	removeBias( &interpolatedvals, &interpolatedvals, false );
	variogramvals_->set( nrcontribwells, 0, 0 );
	variogramnms_->add(grpnames.get( igroup-1 ));
	for( int lag=step; lag<=range; lag+=step )
	{
	    Stats::CalcSetup rcsetuptmp;
	    rcsetuptmp.require( Stats::Average );
	    rcsetuptmp.require( Stats::Variance );
	    rcsetuptmp.require( Stats::Count );
	    Stats::RunCalc<double> statstmp( rcsetuptmp );
	    int idz=0;
	    while ( idz < nrout-(lag/step+1) )
	    {
		double val1 = interpolatedvals.get(idz);
		double val2 = interpolatedvals.get(idz+lag/step);
		if ( lag==step )
		{
		    if ( idz < nrout-3 )
		    {
			statstot+=val1;
		    }
		    else
		    {
			statstot+=val2;
		    }
		}
		idz++;
		if ( mIsZero(val1-val2,1e-6) )
		    continue;
		double diffval = 0.5*(val2-val1)*(val2-val1);
		statstmp+=diffval;
	    }
	    variogramvals_->set(nrcontribwells, lag/step, statstmp.average());
	    variogramstds_->set(nrcontribwells,lag/step-1, statstmp.variance());
	    variogramfolds_->set(nrcontribwells, lag/step-1, statstmp.count());
	}
    }

    double totvar = statstot.variance();

    if ( mIsZero( totvar , 1e-6 ) )
    {
	BufferString emsg = "Failed to compute the total variance\n";
	emsg += "Please check the input data";
	uiMSG().error( emsg.buf() );
	return false;
    }
    if ( statstot.count() <  fold*range/step )
    {
	BufferString emsg ="Did not collect enough data for analysis\n";
	emsg += "Collect more data or decrease fold";
	uiMSG().error( emsg.buf() );
	return false;
    }
    if ( nrcontribwells < nrwells )
    {
	BufferString wmsg ="Warning!\nOnly ";
	wmsg += nrcontribwells;
	wmsg += " out of ";
	wmsg += nrwells;
	wmsg += " well(s) contributed to the output";
	uiMSG().warning( wmsg.buf() );
    }

    for( int lag=step; lag<=range; lag+=step )
    {
	double tmpval = 0;
	double tmpstd = 0;
	od_int64 tmpcount = 0;
	for ( int iwell=1; iwell<=nrcontribwells; iwell++)
	{
	    variogramvals_->set( iwell, lag/step,
		    variogramvals_->get(iwell,lag/step)/totvar);

	    tmpval+=variogramvals_->get(iwell,lag/step);
	    tmpstd+=variogramstds_->get(iwell,lag/step-1);
	    tmpcount+=variogramfolds_->get(iwell,lag/step-1);
	}
	variogramvals_->set( 0, lag/step, tmpval/(double)nrcontribwells);
	variogramstds_->set( 0, lag/step-1, tmpstd/(double)nrcontribwells);
	variogramfolds_->set(0, lag/step-1, tmpcount);
    }

    return true;
}

Array2D<float>* VertVariogramComputer::getData() const
{
    return variogramvals_;
}

Array2D<float>* VertVariogramComputer::getStd() const
{
    return variogramstds_;
}
Array2D<od_int64>* VertVariogramComputer::getFold() const
{
    return variogramfolds_;
}

BufferStringSet* VertVariogramComputer::getLabels() const
{
    return variogramnms_;
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
					 BufferStringSet* labels,
					 int maxrg, int step, bool ishor )
    	: uiDialog(p,uiDialog::Setup("Variogram analysis","Variogram analysis",
		    		     mTODOHelpID ).modal(false))
	, maxrg_(maxrg)
	, step_(step)
{
    if ( !data || !labels ) return;

    data_ = new Array2DImpl<float>( *data );
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
    int size = data_->info().getSize(0);
    for ( int icomp=0; icomp < size; icomp++ )
    {
	for ( int ilag=0; ilag <=maxrg_; ilag+=step_ )
	{
	    float tmpval = data_->get(icomp,ilag/step_);
	    if ( tmpval>maxdataval)
		maxdataval = tmpval;
	    if ( icomp == 0 && ilag == maxrg_ )
		maxdatavalcomp1 = maxdataval;
	}
    }

    labelChangedCB(0);

    disp_->setup().yrg_.stop = maxdataval*1.1;

    rangefld_->sldr()->setMaxValue( maxrg_ );
    rangefld_->sldr()->setStep( step_/100 );
    rangefld_->sldr()->setValue( maxrg_/4 );

    sillfld_->sldr()->setMinValue( 0 );
    sillfld_->sldr()->setMaxValue( maxdataval*1.1 );
    sillfld_->sldr()->setStep( maxdataval/1000 );
    sillfld_->sldr()->setValue( maxdatavalcomp1 );

    fieldChangedCB(0);
}

void uiVariogramDisplay::labelChangedCB( CallBacker* c )
{
    TypeSet<float> xaxisvals;
    for ( int ilag=0; ilag <=maxrg_; ilag+=step_ )
	xaxisvals += ilag;

    int size = xaxisvals.size();
    int curcomp = labelfld_->getIntValue();
    Array1DImpl<float> out(size);
    for ( int idx=0; idx<size; idx++ )
	out.set( idx, data_->get(curcomp,idx) );
    disp_->setVals( xaxisvals.arr(), out.getData(), size );
}

void uiVariogramDisplay::fieldChangedCB( CallBacker* c )
{
    float nugget = 0;
    TypeSet<float> xaxisvals;
    for ( int ilag=0; ilag <=maxrg_; ilag+=step_ )
	xaxisvals += ilag;

    int size = xaxisvals.size();
    float sill = sillfld_->sldr()->getValue();
    float range = rangefld_->sldr()->getValue();
    Array1DImpl<float> out(size);
    getVariogramModel( typefld_->text(), nugget, sill, range, size,
	   		xaxisvals.arr(), out.getData() );
    disp_->setY2Vals( xaxisvals.arr(), out.getData(), size );
}

uiVariogramDisplay::~uiVariogramDisplay()
{
    if ( data_ ) delete data_;
    if ( labels_ ) delete labels_;
}
