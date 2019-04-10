/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/


#include "seisvolprovider.h"
#include "seislineprovider.h"
#include "seisps2dprovider.h"
#include "seisps3dprovider.h"
#include "seisfetcher.h"
#include "seis2ddata.h"

#include "cubesubsel.h"
#include "linesubsel.h"
#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "scaler.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisrawtrcsseq.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survgeom2d.h"
#include "uistrings.h"


Seis::Provider* Seis::Provider::create( Seis::GeomType gt )
{
    switch ( gt )
    {
    case Vol:
	return new VolProvider;
    case VolPS:
	return new PS3DProvider;
    case Line:
	return new LineProvider;
    case LinePS:
	return new PS2DProvider;
    }

    pFreeFnErrMsg( "impossible?" );
    return 0;
}


Seis::Provider* Seis::Provider::create( const IOObj& ioobj, uiRetVal* uirv )
{
    return create( ioobj.key(), uirv );
}


Seis::Provider* Seis::Provider::create( const DBKey& dbky, uiRetVal* uirv )
{
    SeisIOObjInfo objinf( dbky );
    Provider* ret = 0;
    if ( !objinf.isOK() )
    {
	if ( uirv )
	    uirv->set( uiStrings::phrCannotFindDBEntry(dbky) );
    }
    else
    {
	ret = create( objinf.geomType() );
	uiRetVal dum; if ( !uirv ) uirv = &dum;
	*uirv = ret->setInput( dbky );
	if ( !uirv->isOK() )
	    { delete ret; ret = 0; }
    }

    return ret;
}


Seis::Provider* Seis::Provider::create( const IOPar& iop, uiRetVal* uirv )
{
    const DBKey dbky = DBKey::getFromStr( iop.find(sKey::ID()) );
    if ( dbky.isInvalid() )
	return 0;

    Provider* ret = create( dbky, uirv );
    if ( ret )
	ret->usePar( iop );

    return ret;
}


DBKey Seis::Provider::dbKey( const IOPar& iop )
{
    const char* res = iop.find( sKey::ID() );
    BufferString tmp;
    if ( !res )
    {
	res = iop.find( sKey::Name() );
	if ( res && *res )
	{
	    const IOObj* tryioobj = DBM().getByName(IOObjContext::Seis,res);
	    if ( !tryioobj )
		res = 0;
	    else
	    {
		tmp = tryioobj->key();
		res = tmp.buf();
	    }
	}
    }

    if ( res && *res )
	return DBKey::getFromStr( res );

    return DBKey::getInvalid();
}


Seis::Provider::~Provider()
{
    delete seldata_;
}


Seis::Provider2D* Seis::Provider::as2D()
{
    return is2D() ? static_cast<Provider2D*>( this ) : nullptr;
}


const Seis::Provider2D* Seis::Provider::as2D() const
{
    return is2D() ? static_cast<const Provider2D*>( this ) : nullptr;
}


Seis::Provider3D* Seis::Provider::as3D()
{
    return is2D() ? nullptr : static_cast<Provider3D*>( this );
}


const Seis::Provider3D* Seis::Provider::as3D() const
{
    return is2D() ? nullptr : static_cast<const Provider3D*>( this );
}


Seis::Fetcher& Seis::Provider::gtFetcher() const
{
    if ( is2D() )
	return as2D()->fetcher();
    return as3D()->fetcher();
}


uiRetVal Seis::Provider::setInput( const DBKey& dbky )
{
    Threads::Locker locker( lock_ );
    dbky_ = dbky;
    deleteAndZeroPtr( ioobj_ );
    gtFetcher().reset();

    uiRetVal uirv;
    ioobj_ = getIOObj( dbky_ );
    if ( !ioobj_ )
    {
	if ( !dbky_.isValid() )
            uirv = uiStrings::phrDBIDNotValid();
        else
            uirv = uiStrings::phrCannotFindDBEntry( dbky_ );
    }
    else
    {
	establishGeometry( uirv );
	if ( !uirv.isOK() )
	    state_ = NeedInput;
	else
	{
	    scanPositions();
	    state_ = NeedPrep;
	}
    }

    return uirv;
}


void Seis::Provider::reportSetupChg()
{
    if ( state_ == Active )
	state_ = NeedPrep;
    totalnr_ = -1;
}


void Seis::Provider::ensurePositionsScanned() const
{
    (void)totalNr();
}


od_int64 Seis::Provider::totalNr() const
{
    if ( totalnr_ < 0 )
    {
	Threads::Locker locker( lock_ );
	if ( totalnr_ < 0 )
	    scanPositions();
    }
    return totalnr_;
}


void Seis::Provider::getCurPosition( TrcKey& tk ) const
{
    if ( is2D() )
	tk.setPos( curGeomID(), as2D()->curTrcNr() );
    else
	tk.setPos( as3D()->curBinID() );
}


bool Seis::Provider::isPresent( const TrcKey& tk ) const
{
    return is2D() ? as2D()->isPresent( tk.geomID(), tk.trcNr() )
		  : as3D()->isPresent( tk.binID() );
}


void Seis::Provider::getComponentInfo( BufferStringSet& nms,
					   DataType* pdt ) const
{
    nms.setEmpty(); DataType dtype;
    gtComponentInfo( nms, dtype );
    if ( pdt )
	*pdt = dtype;
}


void Seis::Provider::getFallbackComponentInfo( BufferStringSet& nms,
						DataType& dt )
{
    nms.add( sKey::Data() );
    dt = UnknownData;
}


bool Seis::Provider::haveSelComps() const
{
    for ( int idx=0; idx<selcomps_.size(); idx++ )
	if ( selcomps_[idx] >= 0 )
	    return true;
    return false;
}


const Survey::HorSubSel& Seis::Provider::horSubSel( idx_type idx ) const
{
    return fullSubSel( idx ).horSubSel();
}


const Pos::ZSubSel& Seis::Provider::zSubSel( idx_type idx ) const
{
    return fullSubSel( idx ).zSubSel();
}


const Survey::FullSubSel& Seis::Provider::fullSubSel( idx_type idx ) const
{
    if ( is2D() )
	return as2D()->lineSubSel( idx );
    return as3D()->cubeSubSel();
}


void Seis::Provider::setSelData( SelData* sd )
{
    Threads::Locker locker( lock_ );
    if ( seldata_ != sd )
    {
	delete seldata_;
	seldata_ = sd;
	reportSetupChg();
    }
}


void Seis::Provider::selectComponent( int icomp )
{
    Threads::Locker locker( lock_ );
    const auto curnrsel = selcomps_.size();
    if ( curnrsel == 1 && selcomps_[0] == icomp )
	return;

    selcomps_.setEmpty();
    selcomps_ += icomp;
    reportSetupChg();
}


void Seis::Provider::selectComponents( const TypeSet<int>& comps )
{
    Threads::Locker locker( lock_ );
    if ( comps == selcomps_ )
	return;

    selcomps_ = comps;
    reportSetupChg();
}


void Seis::Provider::forceFPData( bool yn )
{
    Threads::Locker locker( lock_ );
    forcefpdata_ = yn; // needs lock but is postproc so no reset needed
}


void Seis::Provider::setReadMode( ReadMode rm )
{
    Threads::Locker locker( lock_ );
    if ( readmode_ != rm )
    {
	readmode_ = rm;
	reportSetupChg();
    }
}


void Seis::Provider::fillPar( IOPar& iop ) const
{
    iop.set( sKey::ID(), dbKey() );
    if ( !seldata_ )
	Seis::SelData::removeFromPar( iop, sKey::Subsel() );
    else
    {
	IOPar sdpar;
	seldata_->fillPar( sdpar );
	iop.mergeComp( sdpar, sKey::Subsel() );
    }

#define mSetParIf( cond, ky, val ) \
    if ( !(cond) ) \
	iop.removeWithKey( ky ); \
    else \
	iop.set( ky, val )

    mSetParIf( forcefpdata_, sKeyForceFPData(), forcefpdata_ );
    mSetParIf( !selcomps_.isEmpty(), sKey::Component(mPlural), selcomps_ );
    mSetParIf( readmode_!=Seis::Prod, sKey::Mode(), (int)readmode_ );
    TrcKey trcky; getCurPosition( trcky );
    if ( trcky.exists() )
	iop.set( sKey::Position(), trcky );
}


uiRetVal Seis::Provider::usePar( const IOPar& iop )
{
    const DBKey dbkey = dbKey( iop );
    if ( !dbkey.isValid() )
	return uiStrings::phrDBIDNotValid();

    Threads::Locker locker( lock_ );

    uiRetVal uirv;
    if ( dbkey != dbKey() )
    {
	uirv = setInput( dbkey );
	if ( !uirv.isOK() )
	    return uirv;
    }

    PtrMan<IOPar> subpar = iop.subselect( sKey::Subsel() );
    setSelData( subpar ? Seis::SelData::get(*subpar) : nullptr );

    forcefpdata_ = iop.isTrue( sKeyForceFPData() );
    iop.get( sKey::Component(mPlural), selcomps_ );
    int rm = readmode_;
    iop.get( sKey::Mode(), rm );
    readmode_ = (Seis::ReadMode)rm;
    TrcKey trcky;
    if ( iop.get(sKey::Position(),trcky) )
	goTo( trcky ); // don't pass uirv - not a real cause for error

    reportSetupChg();
    return uirv;
}


void Seis::Provider::wrapUpGet( TraceData& td, uiRetVal& uirv ) const
{
    if ( uirv.isOK() )
    {
	if ( forcefpdata_ )
	    td.convertToFPs();
	nrdone_++;
    }
}


void Seis::Provider::wrapUpGet( SeisTrc& trc, uiRetVal& uirv ) const
{
    wrapUpGet( trc.data(), uirv );
}


void Seis::Provider::wrapUpGet( SeisTrcBuf& tbuf, uiRetVal& uirv ) const
{
    if ( uirv.isOK() )
    {
	for ( int idx=0; idx<tbuf.size(); idx++ )
	    tbuf.get(idx)->data().convertToFPs();
	nrdone_++;
    }
}


bool Seis::Provider::prepareAccess( uiRetVal& uirv ) const
{
    if ( state_ == Active )
	return true;

    uirv.setOK();
    if ( state_ == NeedInput )
    {
	pErrMsg( "previously, !isOK() on uiRetVal was ignored" );
	uirv = uiStrings::phrDBIDNotValid();
    }
    else
    {
	gtFetcher().reset();
	prepWork( uirv );
	if ( uirv.isOK() )
	    state_ = Active;
    }

    return uirv.isOK();
}


void Seis::Provider::putTraceInGather( const SeisTrc& trc, SeisTrcBuf& tbuf )
{
    const int nrcomps = trc.data().nrComponents();
    const int trcsz = trc.size();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	SeisTrc* newtrc = new SeisTrc( trcsz,
			    trc.data().getInterpreter(icomp)->dataChar() );
	newtrc->info() = trc.info();
	newtrc->info().offset_ = icomp * 100.f;
	newtrc->data().copyFrom( trc.data(), icomp, 0 );
	tbuf.add( newtrc );
    }
}


void Seis::Provider::putGatherInTrace( const SeisTrcBuf& tbuf, SeisTrc& trc )
{
    const int nrcomps = tbuf.size();
    if ( nrcomps < 1 )
	return;

    trc.info() = tbuf.get(0)->info();
    trc.info().offset_ = 0.f;
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	const SeisTrc& buftrc = *tbuf.get( icomp );
	trc.data().addComponent( buftrc.size(),
			      buftrc.data().getInterpreter(0)->dataChar() );
	trc.data().copyFrom( buftrc.data(), 0, icomp );
    }
}


bool Seis::Provider::prepGoTo( uiRetVal* uirv ) const
{
    if ( uirv )
    {
	prepareAccess( *uirv );
	return uirv->isOK();
    }
    else
    {
	uiRetVal tmpuirv;
	prepareAccess( tmpuirv );
	return tmpuirv.isOK();
    }
}


bool Seis::Provider::atValidPos() const
{
    Threads::Locker locker( lock_ );
    return gtAtValidPos();
}


bool Seis::Provider::goTo( const TrcKey& tk, uiRetVal* uirv ) const
{
    if ( is2D() )
	return as2D()->goTo( tk.geomID(), tk.trcNr(), uirv );
    else
	return as3D()->goTo( tk.binID(), uirv );
}


uiRetVal Seis::Provider::getCurrent( SeisTrc& trc ) const
{
    return getTrc( trc, false );
}


uiRetVal Seis::Provider::getNext( SeisTrc& trc ) const
{
    while ( true )
    {
	auto ret = getTrc( trc, true );
	if ( !isNotPresent(ret) )
	    return ret;
    }
    pErrMsg( "Should not reach" );
    return uiRetVal();
}


uiRetVal Seis::Provider::getTrc( SeisTrc& trc, bool next ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !prepareAccess(uirv) )
	return uirv;
    else if ( (next || !gtAtValidPos()) && !moveToNextPosition(uirv) )
	return uirv;

    if ( !isPS() )
	gtCur( trc, uirv );
    else
    {
	SeisTrcBuf tbuf( true );
	gtCurGather( tbuf, uirv );
	putGatherInTrace( tbuf, trc );
    }

    wrapUpGet( trc, uirv );
    return uirv;
}


uiRetVal Seis::Provider::getCurrentGather( SeisTrcBuf& tbuf ) const
{
    return getGath( tbuf, false );
}


uiRetVal Seis::Provider::getNextGather( SeisTrcBuf& tbuf ) const
{
    while ( true )
    {
	auto ret = getGath( tbuf, true );
	if ( !isNotPresent(ret) )
	    return ret;
    }
    pErrMsg( "Should not reach" );
    return uiRetVal();
}


uiRetVal Seis::Provider::getGath( SeisTrcBuf& tbuf, bool next ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !prepareAccess(uirv) )
	return uirv;
    else if ( (next || !gtAtValidPos()) && !moveToNextPosition(uirv) )
	return uirv;

    if ( isPS() )
	gtCurGather( tbuf, uirv );
    else
    {
	SeisTrc trc;
	gtCur( trc, uirv );
	if ( uirv.isOK() )
	    putTraceInGather( trc, tbuf );
    }
    locker.unlockNow();

    wrapUpGet( tbuf, uirv );
    return uirv;
}


uiRetVal Seis::Provider::getAt( const TrcKey& trcky, SeisTrc& trc ) const
{
    uiRetVal uirv;
    if ( !isPS() )
	getSingleAt( trcky, trc.data(), trc.info(), uirv );
    else
    {
	SeisTrcBuf tbuf( true );
	uirv = getGatherAt( trcky, tbuf );
	if ( uirv.isOK() )
	    putGatherInTrace( tbuf, trc );
    }
    return uirv;
}


void Seis::Provider::getSingleAt( const TrcKey& trcky, TraceData& td,
				    SeisTrcInfo& ti, uiRetVal& uirv ) const
{
    Threads::Locker locker( lock_ );
    if ( !prepareAccess(uirv) )
	return;

    if ( !isPS() )
    {
	if ( is2D() )
	    as2D()->gtAt( trcky.geomID(), trcky.trcNr(), td, ti, uirv );
	else
	    as3D()->gtAt( trcky.binID(), td, ti, uirv );
    }
    else
    {
	SeisTrcBuf tbuf( true );
	if ( is2D() )
	    as2D()->gtGatherAt( trcky.geomID(), trcky.trcNr(), tbuf, uirv );
	else
	    as3D()->gtGatherAt( trcky.binID(), tbuf, uirv );
	if ( !uirv.isOK() )
	    return;
	else if ( tbuf.isEmpty() )
	    { uirv.set( uiStrings::phrPosNotFound(trcky) ); return; }
	else
	{
	    SeisTrc trc;
	    putGatherInTrace( tbuf, trc );
	    td = trc.data();
	    ti = trc.info();
	}
    }
    locker.unlockNow();

    wrapUpGet( td, uirv );
}


uiRetVal Seis::Provider::getGatherAt( const TrcKey& trcky,
				      SeisTrcBuf& tbuf ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !prepareAccess(uirv) )
	return uirv;

    if ( isPS() )
    {
	if ( is2D() )
	    as2D()->gtGatherAt( trcky.geomID(), trcky.trcNr(), tbuf, uirv );
	else
	    as3D()->gtGatherAt( trcky.binID(), tbuf, uirv );
    }
    else
    {
	SeisTrc trc;
	if ( is2D() )
	    as2D()->gtAt( trcky.geomID(), trcky.trcNr(),
			    trc.data(), trc.info(), uirv );
	else
	    as3D()->gtAt( trcky.binID(), trc.data(), trc.info(), uirv );
	if ( uirv.isOK() )
	    putTraceInGather( trc, tbuf );
    }
    locker.unlockNow();

    wrapUpGet( tbuf, uirv );
    return uirv;
}


uiRetVal Seis::Provider::getNextSequence( Seis::RawTrcsSequence& rawseq ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !prepareAccess(uirv) )
	return uirv;
    else if ( !moveToNextPosition(uirv) )
	return uirv;

    fillSequence( rawseq, uirv );
    nrdone_ += rawseq.nrPositions();
    locker.unlockNow();

    return uirv;
}


void Seis::Provider::fillSequence( Seis::RawTrcsSequence& rawseq,
				    uiRetVal& uirv ) const
{
    const bool isps = rawseq.isPS();
    if ( isPS() != isps )
	{ pErrMsg("Provider type != seq type"); return; }

    SeisTrc trc;
    SeisTrcInfo& trcinfo = trc.info();
    SeisTrcBuf tbuf( true );
    for ( int ipos=0; ipos<rawseq.nrpos_; ipos++ )
    {
	const TrcKey& tk = (*rawseq.tks_)[ipos];
	if ( isps )
	{
	    uirv = getGatherAt( tk, tbuf );
	    rawseq.copyFrom( tbuf );
	}
	else
	{
	    getSingleAt( tk, *rawseq.data_.get(ipos), trcinfo, uirv );
	    const auto* trl = curTransl();
	    if ( trl )
		rawseq.setTrcScaler( ipos, trl->curtrcscale_ );
	    if ( !uirv.isOK() )
	    {
		uirv = getAt( tk, trc );
		rawseq.copyFrom( trc, &ipos );
	    }
	}
    }
}



Seis::Provider3D::Provider3D()
    : cubedata_(*new CubeData)
    , css_(*new CubeSubSel)
    , cdp_(*new CubeDataPos)
{
}


Seis::Provider3D::~Provider3D()
{
    delete &cdp_;
    delete &css_;
    delete &cubedata_;
}


void Seis::Provider3D::establishGeometry( uiRetVal& uirv ) const
{
    cdp_.toPreStart();
    cubedata_.setEmpty();
    getLocationData( uirv );
}


bool Seis::Provider3D::isPresent( const BinID& bid ) const
{
    return cubedata_.includes( bid );
}


void Seis::Provider3D::getGeometryInfo( CubeData& cd ) const
{
    cd = cubedata_;
}


bool Seis::Provider3D::gtAtValidPos() const
{
    return cdp_.isValid();
}


bool Seis::Provider3D::goTo( const BinID& bid, uiRetVal* uirv ) const
{
    Threads::Locker locker( lock_ );
    return prepGoTo(uirv) ? doGoTo( bid, uirv ) : false;
}


uiRetVal Seis::Provider3D::getAt( const BinID& bid, SeisTrc& trc ) const
{
    return Provider::getAt( TrcKey(bid), trc );
}


uiRetVal Seis::Provider3D::getGatherAt( const BinID& bid,
					SeisTrcBuf& tbuf ) const
{
    return Provider::getGatherAt( TrcKey(bid), tbuf );
}


const CubeSubSel& Seis::Provider3D::cubeSubSel() const
{
    ensurePositionsScanned();
    return css_;
}


const CubeHorSubSel& Seis::Provider3D::cubeHorSubSel() const
{
    ensurePositionsScanned();
    return css_.cubeHorSubSel();
}


bool Seis::Provider3D::moveToNextPosition( uiRetVal& uirv ) const
{
    while ( cubedata_.toNext(cdp_) )
    {
	const auto selres = gtSelRes( cdp_ );
	if ( selres == 0 )
	    return true;
	else if ( selres % 256 == 2 )
	{
	    if ( !cubedata_.toNextLine(cdp_) )
		break;
	    cdp_.sidx_--;
	}
    }

    uirv.set( uiStrings::sFinished() );
    return false;
}


int Seis::Provider3D::gtSelRes( const CubeDataPos& cdp ) const
{
    if ( !cdp.isValid() )
	return 256 + 2; // both inl and crl impossible
    else if ( !seldata_ )
	return 0;
    return seldata_->selRes( cubedata_.binID(cdp) );
}


BinID Seis::Provider3D::curBinID() const
{
    return cubedata_.binID( cdp_ );
}


void Seis::Provider3D::scanPositions() const
{
    auto& chss = css_.cubeHorSubSel();
    CubeData::pos_steprg_type cdinlrg; cubedata_.getInlRange( cdinlrg );
    chss.setInlRange( cdinlrg );
    CubeData::pos_steprg_type cdcrlrg; cubedata_.getCrlRange( cdcrlrg );
    chss.setCrlRange( cdcrlrg );
    if ( !seldata_ || seldata_->isAll() )
	{ mSelf().totalnr_ = cubedata_.totalSize(); return; }

    totsz_type totnr = 0;
    PosInfo::CubeDataIterator cditer( cubedata_ );
    BinID bid;
    CubeData::pos_rg_type inlrg( mUdf(int), 0 ), crlrg;
    while ( cditer.next(bid) )
    {
	if ( !seldata_->isOK(bid) )
	    continue;
	if ( mIsUdf(inlrg.start) )
	{
	    inlrg.start = inlrg.stop = bid.inl();
	    crlrg.start = crlrg.stop = bid.crl();
	}
	else
	{
	    inlrg.include( bid.inl(), false );
	    crlrg.include( bid.crl(), false );
	}
	totnr++;
    }
    if ( totnr > 0 )
    {
	CubeData::pos_steprg_type inl_steprg( inlrg, cdinlrg.step );
	CubeData::pos_steprg_type crl_steprg( crlrg, cdcrlrg.step );
	if ( seldata_->isRange() )
	{
	    const auto& sdcss = seldata_->asRange()->cubeSubSel();
	    const auto sdinlstep = sdcss.inlRange().step;
	    if ( sdinlstep > inl_steprg.step )
		inl_steprg.step = sdinlstep;
	    const auto sdcrlstep = sdcss.crlRange().step;
	    if ( sdcrlstep > crl_steprg.step )
		crl_steprg.step = sdcrlstep;
	}
	chss.setInlRange( inl_steprg );
	chss.setCrlRange( crl_steprg );
    }

    mSelf().totalnr_ = totnr;
}



Seis::Provider2D::Provider2D()
    : l2dds_(*new Line2DDataSet)
    , lsss_(*new LineSubSelSet)
{
}


Seis::Provider2D::~Provider2D()
{
    delete &l2dds_;
    delete &lsss_;
}


bool Seis::Provider2D::isPresent( GeomID gid ) const
{
    return l2dds_.find( gid );
}


bool Seis::Provider2D::isPresent( GeomID gid, trcnr_type tnr ) const
{
    const auto* l2dd = l2dds_.find( gid );
    return l2dd && l2dd->isPresent( tnr );
}


Seis::Provider2D::idx_type Seis::Provider2D::indexOf( GeomID gid ) const
{
    return l2dds_.lineIndexOf( gid );
}


Seis::Provider::size_type Seis::Provider2D::nrGeomIDs() const
{
    return l2dds_.size();
}


Pos::GeomID Seis::Provider2D::geomID( idx_type lidx ) const
{
    return l2dds_.validIdx(lidx) ? l2dds_.get(lidx)->geomID() : GeomID();
}


Seis::Provider2D::trcnr_type Seis::Provider2D::trcNrAt( idx_type lidx,
							idx_type tidx ) const
{
    if ( l2dds_.validIdx(lidx) )
    {
	const auto& l2dd = *l2dds_.get( lidx );
	if ( l2dd.validIdx(tidx) )
	    return l2dd.trcNr( tidx );
    }
    return -1;
}


Seis::Provider2D::trcnr_type Seis::Provider2D::curTrcNr() const
{
    return trcNrAt( lineidx_, trcidx_ );
}


void Seis::Provider2D::getGeometryInfo( idx_type lidx, Line2DData& l2dd ) const
{
    if ( l2dds_.validIdx(lidx) )
	l2dd = *l2dds_.get( lidx );
}


bool Seis::Provider2D::gtAtValidPos() const
{
    return lineidx_ >= 0 && trcidx_ >= 0;
}


bool Seis::Provider2D::goTo( GeomID gid, trcnr_type tnr, uiRetVal* uirv ) const
{
    Threads::Locker locker( lock_ );
    return prepGoTo(uirv) ? doGoTo( gid, tnr, uirv ) : false;
}


uiRetVal Seis::Provider2D::getAt( GeomID gid, trcnr_type tnr,
				    SeisTrc& trc ) const
{
    return Provider::getAt( TrcKey(gid,tnr), trc );
}


uiRetVal Seis::Provider2D::getGatherAt( GeomID gid, trcnr_type tnr,
					SeisTrcBuf& tbuf ) const
{
    return Provider::getGatherAt( TrcKey(gid,tnr), tbuf );
}


const LineHorSubSel& Seis::Provider2D::lineHorSubSel( idx_type iln ) const
{
    ensurePositionsScanned();
    return lsss_.validIdx(iln) ? lsss_.get(iln)->lineHorSubSel()
			       : LineHorSubSel::empty();
}


const LineSubSel& Seis::Provider2D::lineSubSel( idx_type iln ) const
{
    ensurePositionsScanned();
    return lsss_.validIdx(iln) ? *lsss_.get(iln) : LineSubSel::empty();
}


const LineSubSelSet& Seis::Provider2D::lineSubSelSet() const
{
    ensurePositionsScanned();
    return lsss_;
}


void Seis::Provider2D::establishGeometry( uiRetVal& uirv ) const
{
    lineidx_ = 0; trcidx_ = -1;
    l2dds_.setEmpty();
    fillLineData( uirv );
}


void Seis::Provider2D::fillLineData( uiRetVal& uirv ) const
{
    deleteAndZeroPtr( fetcher().dataset_ );
    fetcher().getLineData( l2dds_ );
    uirv = fetcher().uirv_;
    if ( uirv.isOK() && l2dds_.isEmpty() )
	uirv.set( tr("No data in selected 2D data set") );
}


bool Seis::Provider2D::moveToNextPosition( uiRetVal& uirv ) const
{
    if ( l2dds_.validIdx(lineidx_) )
    {
	while ( fetcher().toNextTrace() )
	{
	    const auto selres = gtSelRes( lineidx_, trcidx_ );
	    if ( selres == 0 )
		return true;
	    else if ( selres % 256 == 2 )
	    {
		lineidx_++;
		if ( lineidx_ >= l2dds_.size() )
		    break;
		trcidx_ = -1;
	    }
	}
    }

    uirv.set( uiStrings::sFinished() );
    return false;
}


int Seis::Provider2D::gtSelRes( idx_type lidx, idx_type tidx ) const
{
    if ( !l2dds_.validIdx(lidx) )
	return 256 + 2; // both geomid and trcnr impossible
    else if ( !seldata_ )
	return 0;
    return seldata_->selRes( geomID(lidx), trcNrAt(lidx,tidx) );
}


void Seis::Provider2D::scanPositions() const
{
    lsss_.setEmpty();
    l2dds_.getSubSel( lsss_ );

    if ( !seldata_ || seldata_->isAll() )
	{ mSelf().totalnr_ = l2dds_.totalNrPositions(); return; }

    totsz_type totnr = 0;
    for ( auto l2dd : l2dds_ )
    {
	const auto geomid = l2dd->geomID();
	auto* lss = lsss_.find( geomid );
	if ( !lss )
	    continue;

	PosInfo::Line2DDataIterator l2diter( *l2dd );
	Line2DData::tracenr_rg_type nrrg( mUdf(int), 0 );
	size_type nrtrcs = 0;
	while ( l2diter.next() )
	{
	    const auto tnr = l2diter.trcNr();
	    if ( !seldata_->isOK(geomid,tnr) )
		continue;
	    if ( mIsUdf(nrrg.start) )
		nrrg.start = nrrg.stop = tnr;
	    else
		nrrg.include( tnr, false );
	    nrtrcs++;
	}

	if ( nrtrcs < 1 )
	    lsss_.removeSingle( lsss_.indexOf(lss) );
	else
	{
	    totnr += nrtrcs;
	    const auto l2ddstep = l2dd->trcNrRange().step;
	    Line2DData::tracenr_steprg_type steprg( nrrg, l2ddstep );
	    if ( seldata_->isRange() )
	    {
		const auto* sdlss
			= seldata_->asRange()->findLineSubSel( geomid );
		if ( sdlss )
		{
		    const auto sdnrstep = sdlss->trcNrRange().step;
		    if ( sdnrstep > steprg.step )
			steprg.step = sdnrstep;
		}
	    }
	    lss->trcNrSubSel().setOutputPosRange( steprg );
	}
    }

    mSelf().totalnr_ = totnr;
}
