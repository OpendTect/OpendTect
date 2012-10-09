/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

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
    , message_("Sorting")
{
}


HorizonSorter::~HorizonSorter()
{
    delete result_;
    delete iterator_;
    deepUnRef( horizons_ );
}


void HorizonSorter::init()
{
    calcBoundingBox();
    totalnr_ = is2d_ ? geomids_.size() : hrg_.nrInl();

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
		const Geometry::Horizon2DLine* geom =
		    			hor2d->geometry().sectionGeometry(sid);
		if ( !geom ) continue;

		PosInfo::GeomID geomid = hor2d->geometry().lineGeomID( ldx );
		const int lidx = geomids_.indexOf( geomid );
		const int rowidx =
		    geom->getRowIndex( hor2d->geometry().lineGeomID(ldx) );
		if ( lidx < 0 )
		{
		    geomids_ += geomid;
		    trcrgs_ += geom->colRange( rowidx );
		}
		else
		    trcrgs_[lidx].include( geom->colRange(rowidx) );
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


const char* HorizonSorter::message() const	{ return message_; }

const char* HorizonSorter::nrDoneText() const	{ return "Positions done"; }

od_int64 HorizonSorter::nrDone() const		{ return nrdone_; }

od_int64 HorizonSorter::totalNr() const		{ return totalnr_; }


#define mErrRet(msg)	{ message_ = msg; return ErrorOccurred(); }

int HorizonSorter::nextStep()
{
    if ( !nrdone_ )
    {
	PtrMan<Executor> horreader = EM::EMM().objectLoader( unsortedids_ );
	if ( horreader ) horreader->execute();

	for ( int idx=0; idx<unsortedids_.size(); idx++ )
	{
	    EM::ObjectID objid = EM::EMM().getObjectID( unsortedids_[idx] );
	    EM::EMObject* emobj = EM::EMM().getObject( objid );
	    if ( !emobj )
		mErrRet( "Could not load all horizons" );

	    emobj->ref();
	    mDynamicCastGet(EM::Horizon*,horizon,emobj);
	    if ( !horizon )
	    {
		emobj->unRef();
		mErrRet( "Loaded object is not a horizon" );
	    }
	    horizons_ += horizon;
	}

	init();
    }

    if ( !is2d_ && !iterator_ ) return Finished();

    if ( is2d_ && geomids_.isEmpty() )
	mErrRet( "Could not load 2D geometry." );

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
	       || ( is2d_ && binid_.inl >= geomids_.size() ) )
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
	    EM::SubID subid = binid_.toInt64();
	    if ( is2d_ )
	    {	
		mDynamicCastGet(EM::Horizon2D*,hor2d,horizons_[idx])
		if ( !hor2d ) continue;

		depths[idx] =
		    hor2d->getPos( sid, geomids_[binid_.inl], binid_.crl ).z;
	    }
	    else
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
