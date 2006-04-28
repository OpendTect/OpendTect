/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2006
 RCS:           $Id: horizonsorter.cc,v 1.4 2006-04-28 15:20:13 cvsnanne Exp $
________________________________________________________________________

-*/

#include "horizonsorter.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "keystrs.h"
#include "ptrman.h"
#include "survinfo.h"


HorizonSorter::HorizonSorter( const TypeSet<MultiID>& ids )
    : Executor("Sort horizons")
    , unsortedids_(ids)
    , totalnr_(0)
    , nrdone_(0)
    , iterator_(0)
    , result_(0)
{
    PtrMan<Executor> horreader = EM::EMM().objectLoader( ids );
    horreader->execute();

    horizons_.erase();
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	EM::ObjectID objid = EM::EMM().getObjectID( ids[idx] );
	EM::EMObject* emobj = EM::EMM().getObject( objid );
	emobj->ref();
	mDynamicCastGet(EM::Horizon*,horizon,emobj);
	if ( !horizon )
	    emobj->unRef();
	else
	    emobj->unRefNoDelete();
	horizons_ += horizon;
    }

    init();
}


HorizonSorter::~HorizonSorter()
{
    delete result_;
    delete iterator_;
}


void HorizonSorter::init()
{
    calcBoundingBox();
    totalnr_ = hrg_.totalNr();

    delete iterator_;
    iterator_ = new HorSamplingIterator( hrg_ );

    delete result_;
    result_ = new Array3DImpl<int>( horizons_.size(), horizons_.size(), 2 );
    for ( int idx=0; idx<result_->info().getTotalSz(); idx++ )
	result_->getData()[idx] = 0;
}


void HorizonSorter::calcBoundingBox()
{
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	StepInterval<int> rrg = horizons_[idx]->geometry().rowRange();
	StepInterval<int> crg = horizons_[idx]->geometry().colRange();
	if ( !idx )
	{
	    hrg_.set( rrg, crg );
	    continue;
	}

	hrg_.include( BinID(rrg.start,crg.start) );
	hrg_.include( BinID(rrg.stop,crg.stop) );
    }
}


void HorizonSorter::sort()
{
    sortedids_ = unsortedids_;
    const int nrhors = unsortedids_.size();
    const int maxnrloops = nrhors * nrhors;
    int nrswaps = 0;
    while ( true )
    {
	nrswaps = 0;
	for ( int idx=0; idx<nrhors; idx++ )
	{
	    for ( int idy=idx+1; idy<nrhors; idy++ )
	    {
		const int nrabove = result_->get( idx, idy, 0 );
		const int nrbelow = result_->get( idx, idy, 1 );
		const int idx0 = sortedids_.indexOf( unsortedids_[idx] );
		const int idx1 = sortedids_.indexOf( unsortedids_[idy] );
		if ( nrbelow > nrabove && idx0 > idx1 ) continue;
		if ( nrbelow < nrabove && idx0 < idx1 ) continue;

		if ( nrbelow > nrabove )
		{
		    MultiID mid = sortedids_[idx0];
		    sortedids_.remove( idx0 );
		    sortedids_.insert( idx1, mid );
		}
		else if ( nrbelow < nrabove )
		{
		    MultiID mid = sortedids_[idx1];
		    sortedids_.remove( idx1 );
		    sortedids_.insert( idx0, mid );
		}
		else
		    continue;

		nrswaps++;
	    }
	}

	if ( nrswaps == 0 )
	    break;
    }
}


void HorizonSorter::getSortedList( TypeSet<MultiID>& ids )
{
    ids = sortedids_;
}


const char* HorizonSorter::message() const	{ return "Sorting"; }

const char* HorizonSorter::nrDoneText() const	{ return "Positions done"; }

int HorizonSorter::nrDone() const		{ return nrdone_; }

int HorizonSorter::totalNr() const		{ return totalnr_; }

int HorizonSorter::nextStep()
{
    if ( !iterator_ ) return Finished;

    if ( !iterator_->next(binid_) )
    {
	sort();
	return Finished;
    }

    const EM::SubID subid = binid_.getSerialized();
    const int nrhors = horizons_.size();
    float depths[nrhors];
    for ( int idx=0; idx<nrhors; idx++ )
    {
	const Coord3 pos = horizons_[idx]->getPos( horizons_[idx]->sectionID(0),
	       					   subid );
	depths[idx] = pos.z;
    }

    for ( int idx=0; idx<nrhors; idx++ )
    {
	if ( mIsUdf(depths[idx]) ) continue;
	for ( int idy=idx+1; idy<nrhors; idy++ )
	{
	    if ( mIsUdf(depths[idy]) ) continue;

	    const int resultidx = depths[idx] <= depths[idy] ? 0 : 1;
	    int val = result_->get( idx, idy, resultidx ); val++;
	    result_->set( idx, idy, resultidx, val );
	}
    }

    nrdone_++;
    return MoreToDo;
}

/*
// HorizonModifier
// TODO: Move to new file

HorizonModifier::HorizonModifier()
    : tophor_(0)
    , bothor_(0)
    , topisstatic_(true)
{
}


HorizonModifier::~HorizonModifier()
{
}


bool HorizonModifier::setHorizons( const MultiID& top, const MultiID& bot )
{
    EM::ObjectID objid = EM::EMM().getObjectID( top );
    mDynamicCastGet(EM::Horizon*,tophor,EM::EMM().getObject(objid))
    tophor_ = tophor;

    objid = EM::EMM().getObjectID( bot );
    mDynamicCastGet(EM::Horizon*,bothor,EM::EMM().getObject(objid))
    bothor_ = bothor;

    if ( tophor_ && bothor_ )
    {
	tophor_->ref();
	bothor_->ref();
    }

    return tophor_ && bothor_;
}


void HorizonModifier::setMode( ModifyMode mode )
{
}


void HorizonModifier::setStaticHorizon( bool top )
{
    topisstatic_ = top;
}


void HorizonModifier::modify()
{
    StepInterval<int> rrg = tophor_->geometry.rowRange();
    StepInterval<int> crg = tophor_->geometry.colRange();
    HorSampling hrg.set( rrg, crg );

    rrg = bothor_->geometry.rowRange();
    crg = bothor_->geometry.colRange();
    hrg.include( BinID(rrg.start,crg.start) );
    hrg.include( BinID(rrg.stop,crg.stop) );

    BinID bid;
    HorSamplingIterator iterator( hrg );
    while ( iterator.next(binid) )
    {
    }
}

*/
