/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2005
-*/
 
static const char* rcsID = "$Id: nladataprep.cc,v 1.1 2005-05-12 14:08:47 cvsbert Exp $";

#include "nladataprep.h"
#include "binidvalset.h"
#include "stats.h"


void NLADataPreparer::limitRange( const Interval<float>& r )
{
    Interval<float> rg( r ); rg.sort(true);
    const float ext = rg.width() * mDefEps;
    rg.widen( ext, false );
    bvs_.removeRange( targetcol_, rg, false );
}


void NLADataPreparer::removeUndefs( bool targetonly )
{
    TypeSet<BinIDValueSet::Pos> poss;
    BinIDValueSet::Pos pos;
    while ( bvs_.next(pos) )
    {
	float* vals = bvs_.getVals( pos );
	if ( targetonly && Values::isUdf(vals[targetcol_]) )
	    poss += pos;
	else
	{
	    for ( int idx=0; idx<bvs_.nrVals(); idx++ )
	    {
		if ( Values::isUdf(vals[idx]) )
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

    TypeSet<float> vals; bvs_.getColumn( targetcol_, vals, false );
    Interval<float> rg( vals[0], vals[0] );
    for ( int idx=1; idx<vals.size(); idx++ )
	rg.include( vals[idx] );

    BinIDValueSet::Pos pos; BinID bid;
    const float rgwdth = rg.width();
    while ( bvs_.next(pos) )
    {
	const float* vals = bvs_.getVals( pos );
	float val = vals[targetcol_];
	if ( mIsUndefined(val) ) continue;
	float relpos = (val-rg.start) / rgwdth;
	relpos *= setup.nrclasses; relpos -= 0.5;
	int clss = mNINT( relpos );
	if ( clss < 0 )			clss = 0;
	if ( clss >= setup.nrclasses )	clss = setup.nrclasses-1;
	bvs_.get( pos, bid );
	bvss[clss]->add( bid, vals );
    }

    Stat_initRandom( 0 );
    bvs_.empty();
    for ( int idx=0; idx<setup.nrclasses; idx++ )
    {
	BinIDValueSet& bvs = *bvss[idx];
	const int totsz = bvs.totalSize();
	if ( totsz < setup.nrptsperclss )
	    addVecs( bvs, setup.nrptsperclss - totsz, setup.noiselvl );
	else
	    removeVecs( bvs, totsz - setup.nrptsperclss );
	bvs_.append( bvs );
    }
    deepErase( bvss );
}


void NLADataPreparer::addVecs( BinIDValueSet& bvs, int nr, float )
{
    const int orgsz = bvs.totalSize();
    if ( orgsz == 0 ) return;

    bvs.allowDuplicateBids( true );
    BinIDValueSet bvsnew( bvs.nrVals(), true );
    BinID bid;
    for ( int idx=0; idx<nr; idx++ )
    {
	int dupidx = Stat_getIndex( orgsz );
	BinIDValueSet::Pos pos = bvs.getPos( dupidx );
	const float* vals = bvs.getVals( pos );
	bvs.get( pos, bid );
	bvsnew.add( bid, vals );
    }
    bvs.append( bvsnew );
}


void NLADataPreparer::removeVecs( BinIDValueSet& bvs, int nr )
{
    if ( nr == 0 ) return;

    TypeSet<BinIDValueSet::Pos> poss;
    const int orgsz = bvs.totalSize();
    BinID bid;

    TypeSet<int> rmidxs;
    while ( rmidxs.size() < nr )
    {
	int rmidx = Stat_getIndex( orgsz );
	if ( rmidxs.indexOf(rmidx) < 0 )
	{
	    rmidxs += rmidx;
	    poss += bvs.getPos( rmidx );
	}
    }
    bvs.remove( poss );
}
