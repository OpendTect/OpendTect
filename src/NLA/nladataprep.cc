/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "nladataprep.h"
#include "binidvalset.h"
#include "statrand.h"


NLADataPreparer::NLADataPreparer( BinIDValueSet& bvs, int tc )
    : bvs_(bvs)
    , targetcol_(tc)
    , gen_(*new Stats::RandGen())
{
}


NLADataPreparer::~NLADataPreparer()
{
    delete &gen_;
}


void NLADataPreparer::limitRange( const Interval<float>& r )
{
    Interval<float> rg( r ); rg.sort(true);
    const float ext = rg.width() * mDefEpsF;
    rg.widen( ext, false );
    bvs_.removeRange( targetcol_, rg, false );
}


void NLADataPreparer::removeUndefs( bool targetonly )
{
    TypeSet<BinIDValueSet::SPos> poss;
    BinIDValueSet::SPos pos;
    while ( bvs_.next(pos) )
    {
	float* vals = bvs_.getVals( pos );
	if ( targetonly && mIsUdf(vals[targetcol_]) )
	    poss += pos;
	else
	{
	    for ( int idx=0; idx<bvs_.nrVals(); idx++ )
	    {
		if ( mIsUdf(vals[idx]) )
		    { poss += pos; break; }
	    }
	}
    }
    bvs_.remove( poss );
}


void NLADataPreparer::balance( const NLADataPreparer::BalanceSetup& setup )
{
    ObjectSet<BinIDValueSet> bvss;
    for ( int idx=0; idx<setup.nrclasses; idx++ )
    {
	BinIDValueSet* newbvs = new BinIDValueSet( 0, true );
	newbvs->copyStructureFrom( bvs_ );
	bvss += newbvs;
    }

    const int nrvals = bvs_.nrVals();
    mAllocLargeVarLenArr( Interval<float>, rgs, nrvals );
    BinIDValueSet::SPos pos; BinID bid;
    bool first_pos = true;
    while ( bvs_.next(pos) )
    {
	const float* vals = bvs_.getVals( pos );
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    if ( first_pos )
		rgs[idx].start_ = rgs[idx].stop_ = vals[idx];
	    else
		rgs[idx].include( vals[idx] );
	}
	first_pos = false;
    }

    const Interval<float> targetrg( rgs[targetcol_] );
    pos = BinIDValueSet::SPos();
    const float targetrgwdth = targetrg.width();
    while ( bvs_.next(pos) )
    {
	const float* vals = bvs_.getVals( pos );
	float val = vals[targetcol_];
	if ( mIsUdf(val) ) continue;

	float relpos = (val-targetrg.start_) / targetrgwdth;
	relpos *= setup.nrclasses; relpos -= 0.5;
	int clss = mNINT32( relpos );
	if ( clss < 0 )			clss = 0;
	if ( clss >= setup.nrclasses )	clss = setup.nrclasses-1;
	bvs_.get( pos, bid );
	bvss[clss]->add( bid, vals );
    }

    bvs_.setEmpty();
    for ( int idx=0; idx<setup.nrclasses; idx++ )
    {
	BinIDValueSet& bvs = *bvss[idx];
	const int totsz = mCast( int, bvs.totalSize() );
	if ( totsz < setup.nrptsperclss )
	{
	    ZGate* rgsptr = rgs.ptr();
	    addVecs( bvs, setup.nrptsperclss - totsz, setup.noiselvl, rgsptr );
	}
	else
	    bvs.randomSubselect( totsz - setup.nrptsperclss );

	bvs_.append( bvs );
    }

    deepErase( bvss );
}


void NLADataPreparer::addVecs( BinIDValueSet& bvs, int nr, float noiselvl,
				const Interval<float>* rgs )
{
    const int orgsz = mCast( int, bvs.totalSize() );
    if ( orgsz == 0 )
	return;

    bvs.allowDuplicateBinIDs( true );
    BinIDValueSet bvsnew( bvs.nrVals(), true );
    BinID bid;
    const int nrvals = bvs.nrVals();
    const bool nonoise = noiselvl < 1e-6 || noiselvl > 1 + 1e-6;
    for ( int idx=0; idx<nr; idx++ )
    {
	const int dupidx = gen_.getIndex( orgsz );
	BinIDValueSet::SPos pos = bvs.getPos( dupidx );
	const float* vals = bvs.getVals( pos );
	bvs.get( pos, bid );
	if ( nonoise )
	    bvsnew.add( bid, vals );
	else
	{
	    mAllocLargeVarLenArr( float, newvals, nrvals );
	    for ( int validx=0; validx<nrvals; validx++ )
	    {
		float wdth = rgs[validx].stop_ - rgs[validx].start_;
		wdth *= noiselvl;
		newvals[validx] = (float) (vals[validx] +
				  ((gen_.get()-0.5) * wdth));
	    }

	    float* newvalsptr = newvals.ptr();
	    bvsnew.add( bid, newvalsptr );
	}
    }
    bvs.append( bvsnew );
}
