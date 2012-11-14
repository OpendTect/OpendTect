/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Arnaud Huck
 * DATE     : Mar 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "variogramcomputers.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "datapointset.h"
#include "executor.h"
#include "interpol1d.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "statrand.h"
#include "statruncalc.h"
#include "survinfo.h"

HorVariogramComputer::HorVariogramComputer( DataPointSet& dpset, int size,
					    int cid, int range, int fold,
					    BufferString& errmsg,
					    bool msgiserror )
    : variogramvals_( new Array2DImpl<float>(3,size) )
    , axes_( new Array2DImpl<float>(3,size) )
    , variogramnms_( new BufferStringSet )
{
    dataisok_ = compVarFromRange( dpset, size, cid, range, fold,
	    			  errmsg, msgiserror );
}


HorVariogramComputer::~HorVariogramComputer()
{
    delete variogramvals_;
    delete axes_;
    delete variogramnms_;
}


bool HorVariogramComputer::compVarFromRange( DataPointSet& dpset, int size,
					    int cid, int range, int fold,
					    BufferString& errmsg,
					    bool msgiserror )
{
    Stats::CalcSetup rcsetuptot;
    rcsetuptot.require( Stats::Variance );
    Stats::RunCalc<double> statstot( rcsetuptot );
    for ( DataPointSet::RowID irow=0; irow<dpset.size(); irow++ )
	statstot+=(double)dpset.getValues( irow )[1];

    float totvar = (float)statstot.variance();

    if ( totvar < 0 || mIsZero(totvar,mDefEps) || mIsUdf(totvar) )
    {
	errmsg = "Failed to compute the total variance\n";
	errmsg += "Please check the input data";
	msgiserror = true;
	return false;
    }

    StepInterval<int> inlrg = dpset.bivSet().inlRange();
    StepInterval<int> crlrg = dpset.bivSet().crlRange();

//    uiTaskRunner execdlg( 0 );
//    ExecutorGroup computer( "Processing variogram" );

    variogramnms_->add("Inline");
    variogramnms_->add("Diagonal");
    variogramnms_->add("Crossline");

    for ( int icomp = 0; icomp < 3 ; icomp++ )
    {
	variogramvals_->set( icomp, 0, 0 );
	axes_->set( icomp, 0, 0);
	for( int ilag=1; ilag<size; ilag++ )
	{
	    Stats::CalcSetup rcsetup;
	    rcsetup.require( Stats::Average );
	    rcsetup.require( Stats::Count );
	    Stats::RunCalc<double> stats( rcsetup );

	    int mininl = inlrg.start;
	    int maxinl = inlrg.stop-ilag;
	    int mincrl = crlrg.start;
	    int maxcrl = crlrg.stop-ilag;

	    variogramvals_->set( icomp, ilag, mUdf(float) );
	    float dx = (float)ilag;
	    if ( icomp == 0 )
		dx *= SI().crlDistance();
	    else if ( icomp == 1 )
		dx *= sqrt(SI().inlDistance()*SI().inlDistance()+
		    SI().crlDistance()*SI().crlDistance());
	    else if ( icomp == 2)
		dx *= SI().inlDistance();
	    axes_->set( icomp, ilag, dx );

	    if ( maxinl < mininl && icomp != 0 ) continue;
	    if ( maxcrl < mincrl && icomp != 2 ) continue;

	    int ifold = 0;
	    int itested = 0;
	    while ( ifold < fold )
	    {
		itested++;
    //              computer.add(itested);
		if ( itested > fold*100 ) break;
		int posinl1 = mininl +
			    mNINT32((maxinl-mininl)*Stats::randGen().get());
		int poscrl1 = mincrl +
			    mNINT32((maxcrl-mincrl)*Stats::randGen().get());
		BinID pos1 = BinID( posinl1, poscrl1 );
		DataPointSet::RowID posval1 = dpset.findFirst(pos1);
		if ( posval1<0 ) continue;

		int posinl2 = posinl1;
		int poscrl2 = poscrl1;
		if ( icomp != 2 )
		    poscrl2 += ilag;
		if ( icomp != 0 )
		    posinl2 += ilag;

		BinID pos2 = BinID( posinl2, poscrl2 );
		DataPointSet::RowID posval2 = dpset.findFirst(pos2);
		if ( posval2<0 ) continue;

		double val1 = (double)dpset.getValues( posval1 )[cid-1];
		double val2 = (double)dpset.getValues( posval2 )[cid-1];
		double diffval = 0.5*(val2-val1)*(val2-val1);
				
		if ( mIsUdf(val1) || mIsUdf(val2) || mIsZero(diffval,mDefEps) ) 
		    continue;
						
		stats += diffval;
		ifold++;
	    }

	    if ( stats.count() == 0 )
		variogramvals_->set( icomp, ilag, mUdf(float) );
	    else
		variogramvals_->set( icomp, ilag,
				     ((float)stats.average())/totvar );
	}
    }

//    execdlg.execute(computer);

    return true;
}


Array2D<float>* HorVariogramComputer::getData() const
{
    return variogramvals_;
}

Array2D<float>* HorVariogramComputer::getXaxes() const
{
    return axes_;
}

BufferStringSet* HorVariogramComputer::getLabels() const
{
    return variogramnms_;
}


//-------------------------------------------------------------------------

VertVariogramComputer::VertVariogramComputer( DataPointSet& dpset, int colid,
					    int step, int range, int fold,
					    int nrgroups,
					    BufferString& errmsg,
					    bool msgiserror )
    : variogramvals_( new Array2DImpl<float>(nrgroups+1,range/step+1) )
    , axes_( new Array2DImpl<float>(nrgroups+1,range/step+1) )
    , variogramstds_( new Array2DImpl<float>(nrgroups+1,range/step) )
    , variogramfolds_( new Array2DImpl<od_int64>(nrgroups+1,range/step) )
    , variogramnms_( new BufferStringSet )
{
    dataisok_ = compVarFromRange( dpset, colid, step, range, fold, nrgroups,
	    errmsg, msgiserror );
}


VertVariogramComputer::~VertVariogramComputer()
{
    delete variogramvals_;
    delete axes_;
    delete variogramstds_;
    delete variogramfolds_;
    delete variogramnms_;
}


bool VertVariogramComputer::compVarFromRange( DataPointSet& dpset, int colid,
					    int step, int range, int fold,
					    int nrgroups,
					    BufferString& errmsg,
					    bool msgiserror )
{
    DataPointSet::ColID dpcolid(colid);
    Stats::CalcSetup rcsetuptot;
    rcsetuptot.require( Stats::Variance );
    rcsetuptot.require( Stats::Count );
    Stats::RunCalc<double> statstot( rcsetuptot );
    int nrwells = 0;
    int nrcontribwells = 0;
    float zstep = SI().zIsTime() ? 1000.f : 1.f;

    variogramvals_->set( 0, 0, 0 );
    axes_->set( 0, 0, 0);
    variogramnms_->add("AllWells");

    BufferStringSet grpnames;
    dpset.dataSet().pars().get( "Groups", grpnames );

    if ( grpnames.size() == 0 )
    {
	BufferString basegrpnm = "Well";
	for ( int igroup=1; igroup<=nrgroups; igroup++ )
	{
	    BufferString grpnm = basegrpnm;
	    grpnm += igroup;
	    grpnames.add(grpnm);
	}
	errmsg += "Could not retrieve group names\n";
	errmsg += "Will use generic names";
	msgiserror = false;
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
		if ( !mIsUdf( dpset.value(dpcolid,irow) ) )
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
	    if ( mIsZero( disorder[idz].md_ - disorder[idz+1].md_ , mDefEps ) ) 
	    {
		errmsg = "Data inappropriate for analysis.\n";
		errmsg += "Please re-extract with Radius around wells = 0";
		msgiserror = true;
		return false;
	    }
	}

	int ztop  = mNINT32(dpset.z(disorder[0].rowid_)*zstep)+step;
	int zbase = mNINT32(dpset.z(disorder[nrin-1].rowid_)*zstep)-step;

	if ( zbase >  dpset.z(disorder[nrin-1].rowid_)*zstep ||
	    ztop  <  dpset.z(disorder[0].rowid_)*zstep ||
	    (zbase - ztop) < 3*step )
	{
	    errmsg ="Z interval too small for analysis.\n";
	    errmsg += "Well ";
	    errmsg += grpnames.get( igroup-1 );
	    errmsg += " is not used";
	    msgiserror = false;
	    continue;
	}

	int nrout = (zbase-ztop)/step+1;
	Array1DImpl<double> interpolatedvals(nrout);

	int previdx = 0;
	double depth_out = ztop/zstep;
	for ( int idz = 0; idz<nrout; idz++ )
	{
	    while ( dpset.z(disorder[previdx+1].rowid_) < depth_out )
		previdx++;

	    if ( previdx >= nrin )
	    {
		errmsg ="Interpolation error.\n";
		errmsg += "Well ";
		errmsg += grpnames.get( igroup-1 );
		errmsg += " is not used";
		msgiserror = true;
		continue;
	    }

	    float reldist = ( float ) 
			    ( depth_out-dpset.z(disorder[previdx].rowid_) )/
			    ( dpset.z(disorder[previdx+1].rowid_)-
			      dpset.z(disorder[previdx].rowid_) );
	    double val_out = Interpolate::linearReg1D(
			    dpset.value( dpcolid,disorder[previdx].rowid_ ),
			    dpset.value( dpcolid,disorder[previdx+1].rowid_ ),
			    reldist);
	    interpolatedvals.set(idz,val_out);
	    depth_out += (double)(step/zstep);
	}

	removeBias<double,float>( &interpolatedvals, &interpolatedvals, false );
	variogramvals_->set( nrcontribwells, 0, 0 );
	axes_->set( nrcontribwells, 0, 0);
	variogramnms_->add(grpnames.get( igroup-1 ));
	for ( int lag=step; lag<=range; lag+=step )
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
		double diffval = 0.5*(val2-val1)*(val2-val1);
		idz++;
		if ( mIsZero(diffval,mDefEps) )
		    continue;
		if ( lag==step )
		    statstot+= idz < nrout-3 ? val1 : val2 ;
		statstmp+=diffval;
	    }
	    if ( statstmp.count() == 0 )
	    {
		variogramvals_->set(nrcontribwells, lag/step, mUdf(float));
		variogramstds_->set(nrcontribwells, lag/step-1, mUdf(float));
		variogramfolds_->set(nrcontribwells, lag/step-1, 0);
	    }
	    else
	    {
		variogramvals_->set(nrcontribwells, lag/step,
				    (float)statstmp.average());
		variogramstds_->set(nrcontribwells,lag/step-1,
				    (float)statstmp.variance());
		variogramfolds_->set(nrcontribwells, lag/step-1,
				    (od_int64)statstmp.count());
	    }
	}
    }

    float totvar = (float)statstot.variance();

    if ( totvar < 0 || mIsZero(totvar,mDefEps) || mIsUdf(totvar) )
    {
	errmsg = "Failed to compute the total variance\n";
	errmsg += "Please check the input data";
	msgiserror = true;
	return false;
    }

    if ( statstot.count() < fold*range/step )
    {
	errmsg ="Did not collect enough data for analysis\n";
	errmsg += "Collect more data or decrease fold";
	msgiserror = true;
	return false;
    }

    if ( nrcontribwells < nrwells )
    {
	errmsg ="Warning!\nOnly ";
	errmsg += nrcontribwells;
	errmsg += " out of ";
	errmsg += nrwells;
	errmsg += " well(s) contributed to the output";
	msgiserror = false;
    }

    for ( int lag=step; lag<=range; lag+=step )
    {
	double tmpval = 0;
	double tmpstd = 0;
	od_int64 tmpcount = 0;
	for ( int iwell=1; iwell<=nrcontribwells; iwell++ )
	{
	    variogramvals_->set( iwell, lag/step,
	    variogramvals_->get(iwell,lag/step)/totvar);
	    axes_->set( iwell, lag/step, (float)lag);

	    tmpval+=(double)variogramvals_->get(iwell,lag/step);
	    tmpstd+=(double)variogramstds_->get(iwell,lag/step-1);
	    tmpcount+=variogramfolds_->get(iwell,lag/step-1);
	}

	variogramvals_->set( 0, lag/step, (float)tmpval/(float)nrcontribwells );
	axes_->set( 0, lag/step, (float)lag);
	variogramstds_->set( 0, lag/step-1,(float)tmpstd/(float)nrcontribwells);
	variogramfolds_->set( 0, lag/step-1, (od_int64)tmpcount );
    }

    return true;
}


Array2D<float>* VertVariogramComputer::getData() const
{
    return variogramvals_;
}

Array2D<float>* VertVariogramComputer::getXaxes() const
{
    return axes_;
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
