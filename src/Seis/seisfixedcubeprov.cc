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
#include "seisrangeseldata.h"
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
    : ioobj_(getIOObj(key))
    , trcdist_(SI().crlDistance())
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


#define mErrRet(s) { errmsg_ = s; return false; }

bool SeisFixedCubeProvider::readData( const GeomSubSel& gss,
				      TaskRunner* taskr )
{
    clear();
    const auto geomid = gss.geomID();
    if ( !ioobj_ )
	mErrRet( uiStrings::phrCannotFindDBEntry( uiStrings::sInput() ) );

    if ( ioobj_->isInCurrentSurvey()
	&& Seis::PLDM().isPresent(ioobj_->key(),geomid) )
    {
	auto dp = Seis::PLDM().get<RegularSeisDataPack>(ioobj_->key(),geomid);
	if ( dp && dp->subSel().includes(gss) )
	    { seisdp_ = dp; return true; }
    }

    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( *ioobj_, &uirv );
    if ( !prov )
	mErrRet( uirv );

    gss_ = gss.duplicate();
    const bool is2d = gss_->is2D();
    Seis::RangeSelData* sd = new Seis::RangeSelData( *gss_ );
    trcdist_ = gss_->trcDist( false );

    prov->setSelData( sd );
    data_ = new Array2DImpl<SeisTrc*>(
		    gss_->is2D() ? 1 : gss_->asCubeSubSel()->nrInl(),
		    gss_->is2D() ? gss_->asLineSubSel()->nrTrcs()
				 : gss_->asCubeSubSel()->nrCrl() );
    data_->setAll( nullptr );

    PtrMan<Seis::TrcDataLoader> loader =
	new Seis::TrcDataLoader( *prov, *data_, gss_->horSubSel(), is2d );
    const bool res = TaskRunner::execute( taskr, *loader );
    if ( !res )
	mErrRet( ioobj_->phrCannotReadObj() )

    return true;
}


const SeisTrc* SeisFixedCubeProvider::getTrace( const BinID& bid ) const
{
    if ( !gss_ )
	return nullptr;
    if ( gss_->is2D() )
	{ pErrMsg("2D/3D err"); return nullptr; }
    const auto& css = *gss_->asCubeSubSel();
    if ( !css.includes( bid ) )
	return nullptr;

    if ( seisdp_ )
    {
	SeisTrc& dptrc = dptrc_.getObject();
	seisdp_->fillTrace( bid, dptrc );
	return &dptrc;
    }

    const auto rc = css.rowCol4BinID( bid );
    return data_->get( rc.row(), rc.col() );
}


const SeisTrc* SeisFixedCubeProvider::getTrace( trcnr_type tnr ) const
{
    if ( !gss_ )
	return nullptr;
    if ( !gss_->is2D() )
	{ pErrMsg("2D/3D err"); return nullptr; }
    const auto& lss = *gss_->asLineSubSel();
    if ( !lss.includes(tnr) )
	return nullptr;

    if ( seisdp_ )
    {
	SeisTrc& dptrc = dptrc_.getObject();
	seisdp_->fillTrace( Bin2D(lss.geomID(),tnr), dptrc );
	return &dptrc;
    }

    return data_->get( 0, lss.idx4TrcNr(tnr) );
}


const SeisTrc* SeisFixedCubeProvider::getTrace( const TrcKey& tk ) const
{
    return tk.is2D() ? getTrace( tk.trcNr() ) : getTrace( tk.binID() );
}
