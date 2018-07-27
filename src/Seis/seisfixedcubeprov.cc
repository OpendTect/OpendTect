/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : ?? / Bert
 * DATE     : ?? / July 2017
-*/


#include "seisfixedcubeprov.h"

#include "arrayndimpl.h"
#include "trckeyzsampling.h"
#include "posinfo2d.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seispreload.h"
#include "seisprovider.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "executor.h"
#include "uistrings.h"


namespace Seis
{

class TrcDataLoader : public Executor
{ mODTextTranslationClass(TrcDataLoader);
public:
TrcDataLoader( Seis::Provider& prov, Array2D<SeisTrc*>& arr,
	       const TrcKeySampling& hs, bool is2d )
    : Executor("Data Loader")
    , prov_(prov), arr_(arr), hs_(hs)
    , nrdone_(0)
    , is2d_(is2d)
{
}

od_int64 totalNr() const
{ return hs_.totalNr(); }

od_int64 nrDone() const
{ return nrdone_; }

uiString nrDoneText() const
{ return uiStrings::sPositionsDone(); }

uiString message() const
{ return errmsg_.isEmpty() ? tr("Reading traces") : errmsg_; }

int nextStep()
{
    SeisTrc* trc = new SeisTrc;
    const uiRetVal uirv = prov_.getNext( *trc );
    if ( !uirv.isOK() )
    {
	delete trc;
	if ( isFinished(uirv) )
	    return Finished();

	errmsg_ = uirv;
	return ErrorOccurred();
    }

    const BinID bid = trc->info().binID();
    const int inlidx = is2d_ ? 0 : hs_.inlIdx( bid.inl() );
    const int crlidx = hs_.crlIdx( bid.crl() );
    arr_.set( inlidx, crlidx, trc );
    nrdone_++;
    return MoreToDo();
}

    Seis::Provider&		prov_;
    Array2D<SeisTrc*>&		arr_;
    const TrcKeySampling&	hs_;
    uiString			errmsg_;
    od_int64			nrdone_;
    bool			is2d_;

};

} // namespace Seis



SeisFixedCubeProvider::SeisFixedCubeProvider( const DBKey& key )
    : tkzs_(false)
    , data_(0)
    , ioobj_(getIOObj(key))
    , trcdist_(SI().crlDistance())
    , geomid_(-1)
{
}


SeisFixedCubeProvider::~SeisFixedCubeProvider()
{
    clear();
    delete ioobj_;
}


uiString SeisFixedCubeProvider::errMsg() const
{ return errmsg_; }


void SeisFixedCubeProvider::clear()
{
    errmsg_.setEmpty();
    seisdp_ = 0;
    if ( !data_ )
	return;

    for ( int idx=0; idx<data_->getSize(0); idx++ )
	for ( int idy=0; idy<data_->getSize(1); idy++ )
	    delete data_->get( idx, idy );

    delete data_;
    data_ = 0;
}


bool SeisFixedCubeProvider::isEmpty() const
{
    return !seisdp_ && !data_;
}


bool SeisFixedCubeProvider::calcTrcDist( const Pos::GeomID geomid )
{
    trcdist_ = SI().crlDistance();
    const SeisIOObjInfo si( ioobj_->key() );
    if ( !si.is2D() )
	return true;

    BufferStringSet nms;
    si.getComponentNames( nms, geomid );
    if ( nms.size() > 1 && nms.get(1).isEqual("Line dip",CaseInsensitive) )
    {
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(geomid))
	if ( !geom2d )
	{ errmsg_ = tr("Cannot read 2D geometry"); return false; }

	float max;
	geom2d->data().compDistBetwTrcsStats( max, trcdist_ );
	if ( mIsZero(trcdist_,mDefEps) )
	{
	    errmsg_ = tr("Cannot calculate median trace distance");
	    return false;
	}
    }

    return true;
}


bool SeisFixedCubeProvider::readData(const TrcKeyZSampling& cs,
				     TaskRunner* taskr)
{ return readData( cs, mUdfGeomID, taskr ); }


#define mErrRet(s) { errmsg_ = s; return false; }

bool SeisFixedCubeProvider::readData( const TrcKeyZSampling& cs,
				      const Pos::GeomID geomid,
				      TaskRunner* taskr )
{
    geomid_ = geomid;
    clear();
    if ( !ioobj_ )
	mErrRet( uiStrings::phrCannotFindDBEntry( uiStrings::sInput() ) );

    if ( ioobj_->isInCurrentSurvey()
	&& Seis::PLDM().isPresent(ioobj_->key(),geomid) )
    {
	RegularSeisDataPack* dp =
		Seis::PLDM().getAndCast<RegularSeisDataPack>( ioobj_->key(),
								geomid );
	if ( dp )
	{
	    if ( dp->sampling().includes(cs) )
		{ seisdp_ = dp; return true; }
	}
    }

    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( *ioobj_, &uirv );
    if ( !prov )
	mErrRet( uirv );

    tkzs_ = cs;
    const bool is2d = !mIsUdfGeomID( geomid );
    Seis::RangeSelData* sd = new Seis::RangeSelData( tkzs_ );
    if ( is2d )
    {
	sd->setGeomID( geomid );
	if ( !calcTrcDist(geomid) )
	    return false;
    }

    prov->setSelData( sd );
    data_ = new Array2DImpl<SeisTrc*>( tkzs_.hsamp_.nrInl(),
				       tkzs_.hsamp_.nrCrl() );
    for ( int idx=0; idx<data_->getSize(0); idx++ )
	for ( int idy=0; idy<data_->getSize(1); idy++ )
	    data_->set( idx, idy, 0 );

    PtrMan<Seis::TrcDataLoader> loader =
	new Seis::TrcDataLoader( *prov, *data_, tkzs_.hsamp_, is2d );
    const bool res = TaskRunner::execute( taskr, *loader );
    if ( !res )
	mErrRet( ioobj_->phrCannotReadObj() )

    return true;
}


const SeisTrc* SeisFixedCubeProvider::getTrace( int trcnr ) const
{
    return getTrace( BinID(0,trcnr) );
}


const SeisTrc* SeisFixedCubeProvider::getTrace( const BinID& bid ) const
{
    if ( isEmpty() || !tkzs_.hsamp_.includes(bid) )
	return 0;

    if ( seisdp_ )
    {
	TrcKey tk( bid );
	if ( geomid_ >= 0 )
	    tk.setGeomID( geomid_ );
	SeisTrc& dptrc = dptrc_.getObject();
	seisdp_->fillTrace( TrcKey(bid), dptrc );
	return &dptrc;
    }

    return data_->get( tkzs_.inlIdx(bid.inl()), tkzs_.crlIdx(bid.crl()) );
}
