/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horizonsorter.cc,v 1.12 2009-07-01 06:26:08 cvsraman Exp $";

#include "horizonsorter.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "ptrman.h"
#include "survinfo.h"


HorizonSorter::HorizonSorter( const TypeSet<MultiID>& ids, bool is2d )
    : Executor("Sort horizons")
    , unsortedids_(ids)
    , totalnr_( ids.size() * (SI().inlRange(true).width()+1) )
    , nrdone_(0)
    , iterator_(0)
    , result_(0)
    , is2d_(is2d)
    , linenames_(*new BufferStringSet)
{
}


HorizonSorter::~HorizonSorter()
{
    delete result_;
    delete iterator_;
    delete &linenames_;
    deepUnRef( horizons_ );
}


void HorizonSorter::init()
{
    calcBoundingBox();
    totalnr_ = is2d_ ? linenames_.size() : hrg_.nrInl();

    if ( !is2d_ )
    {
	delete iterator_;
	iterator_ = new HorSamplingIterator( hrg_ );
    }

    delete result_;
    result_ = new Array3DImpl<int>( horizons_.size(), horizons_.size(), 2 );
    for ( int idx=0; idx<result_->info().getTotalSz(); idx++ )
	result_->getData()[idx] = 0;
}


void HorizonSorter::calcBoundingBox()
{
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	if ( is2d_ )
	{
	    mDynamicCastGet(EM::Horizon2D*,hor2d,horizons_[idx])
	    if ( !hor2d ) continue;

	    const int sid = hor2d->sectionID( 0 );
	    for ( int ldx=0; ldx<hor2d->geometry().nrLines(); ldx++ )
	    {
		const int lid = hor2d->geometry().lineID( ldx );
		const char* linenm = hor2d->geometry().lineName( lid );

		const Geometry::Horizon2DLine* geom =
		    			hor2d->geometry().sectionGeometry(sid);
		if ( !geom ) continue;

		const int lidx = linenames_.indexOf(linenm);
		if ( lidx < 0 )
		{
		    linenames_.add( linenm );
		    trcrgs_ += geom->colRange( lid );
		}
		else
		    trcrgs_[lidx].include( geom->colRange(lid) );
	    }

	    continue;
	}

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


int HorizonSorter::getNrCrossings( const MultiID& mid1,
				   const MultiID& mid2 ) const
{
    const int idx1 = unsortedids_.indexOf( mid1 );
    const int idx2 = unsortedids_.indexOf( mid2 );
    const int nrabove = result_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 0 );
    const int nrbelow = result_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 1 );
    return mMIN(nrabove,nrbelow);
}


const char* HorizonSorter::message() const	{ return "Sorting"; }

const char* HorizonSorter::nrDoneText() const	{ return "Positions done"; }

od_int64 HorizonSorter::nrDone() const		{ return nrdone_; }

od_int64 HorizonSorter::totalNr() const		{ return totalnr_; }

int HorizonSorter::nextStep()
{
    if ( !nrdone_ )
    {
	PtrMan<Executor> horreader = EM::EMM().objectLoader( unsortedids_ );
	horreader->execute();

	for ( int idx=0; idx<unsortedids_.size(); idx++ )
	{
	    EM::ObjectID objid = EM::EMM().getObjectID( unsortedids_[idx] );
	    EM::EMObject* emobj = EM::EMM().getObject( objid );
	    if ( !emobj ) return ErrorOccurred();
	    emobj->ref();
	    mDynamicCastGet(EM::Horizon*,horizon,emobj);
	    if ( !horizon )
	    {
		emobj->unRef();
		return ErrorOccurred();
	    }
	    horizons_ += horizon;
	}

	init();
    }

    if ( !is2d_ && !iterator_ ) return Finished();

    const int previnl = binid_.inl;
    while ( binid_.inl==previnl )
    {
	if ( is2d_ )
	{
	    binid_.crl += trcrgs_[previnl].step;
	    if ( binid_.crl > trcrgs_[previnl].stop )
		binid_.inl++;
	}

	if ( ( !is2d_ && !iterator_->next(binid_) )
	       || ( is2d_ && binid_.inl >= linenames_.size() ) )
	{
	    sort();
	    return Finished();
	}

	if ( is2d_ && binid_.inl != previnl )
	    binid_.crl = trcrgs_[binid_.inl].start;

	const int nrhors = horizons_.size();
	ArrPtrMan<float> depths = new float [nrhors];
	for ( int idx=0; idx<nrhors; idx++ )
	{
	    const EM::SectionID sid = horizons_[idx]->sectionID(0);
	    EM::SubID subid = binid_.getSerialized();
	    if ( is2d_ )
	    {	
		mDynamicCastGet(EM::Horizon2D*,hor2d,horizons_[idx])
		if ( !hor2d ) continue;

		const int lidx = hor2d->geometry().lineIndex(
						linenames_.get(binid_.inl) );
		const int lid = hor2d->geometry().lineID( lidx );
		subid = BinID( lid, binid_.crl ).getSerialized();
	    }
	    
	    depths[idx] = horizons_[idx]->getPos( sid, subid ).z;
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
    }

    nrdone_++;
    return MoreToDo();
}
