/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizonsorter.h"

#include "arrayndimpl.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "ptrman.h"
#include "uistrings.h"


HorizonSorter::HorizonSorter( const TypeSet<EM::ObjectID>& ids, bool is2d )
    : Executor("Sort horizons")
    , unsortedids_(ids)
    , is2d_(is2d)
    , msg_(tr("Sorting"))
{
    preinit();
    init();
}


HorizonSorter::HorizonSorter( const TypeSet<MultiID>& keys, bool is2d )
    : Executor("Sort horizons")
    , unsortedkeys_(keys)
    , is2d_(is2d)
    , msg_(tr("Sorting"))
{
    if ( !preinit() || !init() )
	totalnr_ = 1; //ensure reach doPrepare
}


HorizonSorter::~HorizonSorter()
{
    delete resultcount_;
    delete resultzsum_;
    delete iterator_;
    deepUnRef( horizons_ );
}


void HorizonSorter::setTaskRunner( TaskRunner& taskrun )
{
    taskrun_ = &taskrun;
}


bool HorizonSorter::preinit()
{
    RefObjectSet<EM::EMObject> loadedhors;
    if ( unsortedids_.isEmpty() && !unsortedkeys_.isEmpty() )
    {
	PtrMan<Task> horreader = EM::EMM().objectLoader( unsortedkeys_ );
	if ( horreader )
	{
	    if ( !TaskRunner::execute(taskrun_,*horreader.ptr()) )
	    {
		msg_ = horreader->uiMessage();
		return false;
	    }
	}

	for ( const auto& unsortedkey : unsortedkeys_ )
	{
	    const EM::ObjectID id = EM::EMM().getObjectID( unsortedkey );
	    if ( !id.isValid() )
	    {
		msg_ = uiStrings::phrCannotLoad(tr("all horizons"));
		return false;
	    }

	    unsortedids_ += id;
	    loadedhors += EM::EMM().getObject( id );
	}
    }

    const StringView reqtype = is2d_ ? EM::Horizon2D::typeStr()
				     : EM::Horizon3D::typeStr();
    for ( const auto& unsortedid : unsortedids_ )
    {
	RefMan<EM::EMObject> emobj = EM::EMM().getObject( unsortedid );
	if ( !emobj )
	{
	    msg_ = uiStrings::phrCannotLoad(tr("all horizons"));
	    return false;
	}

	if ( reqtype != emobj->getTypeStr() )
	{
	    msg_ = tr("Loaded object is not a %1")
			    .arg( is2d_ ? "2D Horizon" : "3D Horizon" );
	    pErrMsg( toString(msg_) );
	    return false;
	}

	mDynamicCastGet(EM::Horizon*,horizon,emobj.ptr());
	if ( !horizon )
	{
	    msg_ = tr("Loaded object is not a horizon");
	    return false;
	}

	horizons_ += horizon;
    }

    return true;
}


bool HorizonSorter::init()
{
    calcBoundingBox();
    totalnr_ = is2d_ ? geomids_.size() : tks_.nrInl();

    if ( !is2d_ )
    {
	delete iterator_;
	iterator_ = new TrcKeySamplingIterator( tks_ );
	if ( !iterator_ )
	    return false;
    }

    const int nrhors = horizons_.size();
    delete resultcount_;
    resultcount_ = new Array3DImpl<int>( nrhors, nrhors, 2 );
    if ( !resultcount_ || !resultcount_->isOK() )
	return false;

    delete resultzsum_;
    resultzsum_ = new Array3DImpl<double>( nrhors, nrhors, 2 );
    if ( !resultzsum_ || !resultzsum_->isOK() )
	return false;

    resultcount_->setAll( 0 );
    resultzsum_->setAll( 0. );

    return true;
}


void HorizonSorter::calcBoundingBox()
{
    for ( int idx=0; idx<horizons_.size(); idx++ )
    {
	ConstRefMan<EM::Horizon> horizon = horizons_[idx];
	if ( is2d_ )
	{
	    mDynamicCastGet(const EM::Horizon2D*,hor2d,horizon.ptr())
	    if ( !hor2d )
		continue;

	    for ( int ldx=0; ldx<hor2d->geometry().nrLines(); ldx++ )
	    {
		const Geometry::Horizon2DLine* geom =
			hor2d->geometry().geometryElement();
		if ( !geom )
		    continue;

		const Pos::GeomID geomid = hor2d->geometry().geomID( ldx );
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
	}
	else
	{
	    const StepInterval<int> rrg = horizon->geometry().rowRange();
	    const StepInterval<int> crg = horizon->geometry().colRange();
	    if ( idx == 0 )
		tks_.set( rrg, crg );
	    else
	    {
		tks_.include( BinID(rrg.start_,crg.start_) );
		tks_.include( BinID(rrg.stop_,crg.stop_) );
	    }
	}
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
		const double sumabove = resultzsum_->get( idx, idy, 0 );
		const double sumbelow = resultzsum_->get( idx, idy, 1 );
		const int idx0 = sortedids_.indexOf( unsortedids_[idx] );
		const int idx1 = sortedids_.indexOf( unsortedids_[idy] );
		if ( sumbelow > sumabove && idx0 > idx1 )
		    continue;

		if ( sumbelow < sumabove && idx0 < idx1 )
		    continue;

		if ( sumbelow > sumabove )
		{
		    const EM::ObjectID id = sortedids_[idx0];
		    sortedids_.removeSingle( idx0 );
		    sortedids_.insert( idx1, id );
		}
		else if ( sumbelow < sumabove )
		{
		    const EM::ObjectID id = sortedids_[idx1];
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
    const int nrabove = resultcount_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 0);
    const int nrbelow = resultcount_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 1);
    return mMIN(nrabove,nrbelow);
}


int HorizonSorter::getNrCrossings( const EM::ObjectID id1,
				   const EM::ObjectID id2 ) const
{
    const int idx1 = unsortedids_.indexOf( id1 );
    const int idx2 = unsortedids_.indexOf( id2 );
    const int nrabove = resultcount_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 0);
    const int nrbelow = resultcount_->get( mMIN(idx1,idx2), mMAX(idx1,idx2), 1);
    return mMIN(nrabove,nrbelow);
}


uiString HorizonSorter::uiMessage() const	{ return msg_; }

uiString HorizonSorter::uiNrDoneText() const
{
    return ParallelTask::sPosFinished();
}

od_int64 HorizonSorter::nrDone() const		{ return nrdone_; }

od_int64 HorizonSorter::totalNr() const		{ return totalnr_; }


bool HorizonSorter::doPrepare( od_ostream* strm )
{
    if ( !resultcount_ || !resultzsum_ )
	return false;

    if ( is2d_ && geomids_.isEmpty() )
    {
	msg_ = tr("Could not load 2D geometry.");
	return false;
    }
    else if ( !is2d_ && !iterator_ )
    {
	msg_ = tr("Cannot create horizon iterator");
	return false;
    }

    return Executor::doPrepare( strm );
}


#define mErrRet(msg)	{ msg_ = msg; return ErrorOccurred(); }

int HorizonSorter::nextStep()
{
    if ( !is2d_ && !iterator_ )
	return Finished();

    const int previnl = binid_.inl();
    while ( binid_.inl()==previnl )
    {
	if ( is2d_ )
	{
	    binid_.crl() += trcrgs_[previnl].step_;
	    if ( binid_.crl() > trcrgs_[previnl].stop_ )
		binid_.inl()++;
	}

	if ( ( !is2d_ && !iterator_->next(binid_) ) ||
	      ( is2d_ && binid_.inl() >= geomids_.size() ) )
	{
	    sort();
	    return Finished();
	}

	if ( is2d_ && binid_.inl() != previnl )
	    binid_.crl() = trcrgs_[binid_.inl()].start_;

	const int nrhors = horizons_.size();
	mAllocLargeVarLenArr( float, depths, nrhors );
	for ( int idx=0; idx<nrhors; idx++ )
	{
	    ConstRefMan<EM::Horizon> horizon = horizons_[idx];
	    if ( is2d_ )
	    {
		mDynamicCastGet(const EM::Horizon2D*,hor2d,horizon.ptr())
		if ( !hor2d )
		    continue;

		depths[idx] = (float) hor2d->getPos( geomids_[binid_.inl()],
						     binid_.crl() ).z_;
	    }
	    else
	    {
		const EM::SubID subid = binid_.toInt64();
		depths[idx] = (float) horizon->getPos( subid ).z_;
	    }
	}

	for ( int idx=0; idx<nrhors; idx++ )
	{
	    if ( mIsUdf(depths[idx]) )
		continue;

	    for ( int idy=idx+1; idy<nrhors; idy++ )
	    {
		if ( mIsUdf(depths[idy]) ||
		     mIsEqual(depths[idx],depths[idy],mDefEps) )
		    continue;

		const int resultidx = depths[idx] < depths[idy] ? 0 : 1;
		int curcount = resultcount_->get( idx, idy, resultidx );
		curcount++;
		resultcount_->set( idx, idy, resultidx, curcount );

		double cursum = resultzsum_->get( idx, idy, resultidx );
		cursum += resultidx==0 ? depths[idy]-depths[idx]
				       : depths[idx]-depths[idy];
		resultzsum_->set( idx, idy, resultidx, cursum );
	    }
	}
    }

    nrdone_++;
    return MoreToDo();
}


bool HorizonSorter::doFinish( bool success, od_ostream* strm )
{
    return Executor::doFinish( success, strm );
}
