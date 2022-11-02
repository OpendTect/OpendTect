/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizonsorter.h"

#include "arrayndimpl.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "ptrman.h"
#include "uistrings.h"


HorizonSorter::HorizonSorter( const TypeSet<EM::ObjectID>& ids, bool is2d )
    : Executor("Sort horizons")
    , unsortedids_(ids)
    , totalnr_(-1)
    , nrdone_(0)
    , iterator_(0)
    , result_(0)
    , is2d_(is2d)
    , message_(tr("Sorting"))
    , taskrun_(0)
{
}


HorizonSorter::HorizonSorter( const TypeSet<MultiID>& keys, bool is2d )
    : Executor("Sort horizons")
    , unsortedkeys_(keys)
    , totalnr_(-1)
    , nrdone_(0)
    , iterator_(0)
    , result_(0)
    , is2d_(is2d)
    , message_(tr("Sorting"))
    , taskrun_(0)
{
}


HorizonSorter::~HorizonSorter()
{
    delete result_;
    delete iterator_;
    deepUnRef( horizons_ );
}


void HorizonSorter::setTaskRunner( TaskRunner& taskrun )
{
    taskrun_ = &taskrun;
}


void HorizonSorter::init()
{
    calcBoundingBox();
    totalnr_ = is2d_ ? geomids_.size() : tks_.nrInl();

    if ( !is2d_ )
    {
	delete iterator_;
	iterator_ = new TrcKeySamplingIterator( tks_ );
    }

    delete result_;
    result_ = new Array3DImpl<int>( horizons_.size(), horizons_.size(), 2 );
    result_->setAll( 0 );
}


void HorizonSorter::calcBoundingBox()
{
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	if ( is2d_ )
	{
	    mDynamicCastGet(EM::Horizon2D*,hor2d,horizons_[idx])
	    if ( !hor2d ) continue;

	    for ( int ldx=0; ldx<hor2d->geometry().nrLines(); ldx++ )
	    {
		const Geometry::Horizon2DLine* geom =
			hor2d->geometry().geometryElement();
		if ( !geom ) continue;

		Pos::GeomID geomid = hor2d->geometry().geomID( ldx );
		const int lidx = geomids_.indexOf( geomid );
		const int rowidx = geom->getRowIndex( geomid );
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
	    tks_.set( rrg, crg );
	    continue;
	}

	tks_.include( BinID(rrg.start,crg.start) );
	tks_.include( BinID(rrg.stop,crg.stop) );
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
		    EM::ObjectID id = sortedids_[idx0];
		    sortedids_.removeSingle( idx0 );
		    sortedids_.insert( idx1, id );
		}
		else if ( nrbelow < nrabove )
		{
		    EM::ObjectID id = sortedids_[idx1];
		    sortedids_.removeSingle( idx1 );
		    sortedids_.insert( idx0, id );
		}
		else
		    continue;

		nrswaps++;
	    }
	}

	if ( nrswaps == 0 )
	    break;
    }

    if ( !unsortedkeys_.isEmpty() )
    {
	sortedkeys_.setEmpty();
	for ( int idx=0; idx<sortedids_.size(); idx++ )
	{
	    const int oldidx = unsortedids_.indexOf( sortedids_[idx] );
	    sortedkeys_ += unsortedkeys_[oldidx];
	}
    }
}


void HorizonSorter::getSortedList( TypeSet<MultiID>& keys )
{
    keys = sortedkeys_;
}


void HorizonSorter::getSortedList( TypeSet<EM::ObjectID>& ids )
{
    ids = sortedids_;
}


int HorizonSorter::getNrCrossings( const MultiID& mid1,
				   const MultiID& mid2 ) const
{
    const int idx1 = unsortedkeys_.indexOf( mid1 );
    const int idx2 = unsortedkeys_.indexOf( mid2 );
    const int nrabove = result_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 0 );
    const int nrbelow = result_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 1 );
    return mMIN(nrabove,nrbelow);
}


int HorizonSorter::getNrCrossings( const EM::ObjectID id1,
				   const EM::ObjectID id2 ) const
{
    const int idx1 = unsortedids_.indexOf( id1 );
    const int idx2 = unsortedids_.indexOf( id2 );
    const int nrabove = result_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 0 );
    const int nrbelow = result_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 1 );
    return mMIN(nrabove,nrbelow);
}


uiString HorizonSorter::uiMessage() const	{ return message_; }

uiString HorizonSorter::uiNrDoneText() const { return tr("Positions done"); }

od_int64 HorizonSorter::nrDone() const		{ return nrdone_; }

od_int64 HorizonSorter::totalNr() const		{ return totalnr_; }


#define mErrRet(msg)	{ message_ = msg; return ErrorOccurred(); }

int HorizonSorter::nextStep()
{
    if ( !nrdone_ )
    {
	PtrMan<Executor> horreader = nullptr;
	if ( unsortedids_.isEmpty() && !unsortedkeys_.isEmpty() )
	{
	    horreader = EM::EMM().objectLoader( unsortedkeys_ );
	    if ( horreader )
	    {
		if ( taskrun_ )
		    taskrun_->execute( *horreader.ptr() );
		else
		    horreader->execute();
	    }

	    for ( int idx=0; idx<unsortedkeys_.size(); idx++ )
	    {
		const EM::ObjectID id =
				EM::EMM().getObjectID( unsortedkeys_[idx] );
		if ( !id.isValid() )
		    mErrRet( uiStrings::phrCannotLoad(tr("all horizons")) );

		unsortedids_ += id;
	    }
	}

	for ( int idx=0; idx<unsortedids_.size(); idx++ )
	{
	    EM::EMObject* emobj = EM::EMM().getObject( unsortedids_[idx] );
	    if ( !emobj )
		mErrRet( uiStrings::phrCannotLoad(tr("all horizons")) );

	    emobj->ref();
	    mDynamicCastGet(EM::Horizon*,horizon,emobj);
	    if ( !horizon )
	    {
		emobj->unRef();
		mErrRet( tr("Loaded object is not a horizon") );
	    }
	    horizons_ += horizon;
	}

	init();
    }

    if ( !is2d_ && !iterator_ ) return Finished();

    if ( is2d_ && geomids_.isEmpty() )
	mErrRet( tr("Could not load 2D geometry.") );

    const int previnl = binid_.inl();
    while ( binid_.inl()==previnl )
    {
	if ( is2d_ )
	{
	    binid_.crl() += trcrgs_[previnl].step;
	    if ( binid_.crl() > trcrgs_[previnl].stop )
		binid_.inl()++;
	}

	if ( ( !is2d_ && !iterator_->next(binid_) )
	       || ( is2d_ && binid_.inl() >= geomids_.size() ) )
	{
	    sort();
	    return Finished();
	}

	if ( is2d_ && binid_.inl() != previnl )
	    binid_.crl() = trcrgs_[binid_.inl()].start;

	const int nrhors = horizons_.size();
	mAllocLargeVarLenArr( float, depths, nrhors );
	for ( int idx=0; idx<nrhors; idx++ )
	{
	    const EM::SubID subid = binid_.toInt64();
	    if ( is2d_ )
	    {
		mDynamicCastGet(EM::Horizon2D*,hor2d,horizons_[idx])
		if ( !hor2d ) continue;

		depths[idx] =
		    (float) hor2d->getPos( geomids_[binid_.inl()],
					   binid_.crl() ).z;
	    }
	    else
		depths[idx] = (float) horizons_[idx]->getPos( subid ).z;
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
