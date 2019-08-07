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
    const DBKey dbky( iop.find(sKey::ID()) );
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
	return DBKey( res );

    return DBKey();
}


Seis::Provider::Provider( bool is2d )
    : possiblepositions_(is2d ? *new LineCollData
			      : *(LineCollData*)new CubeData)
{
    if ( is2d )
    {
	Survey::Geometry2D::getLineCollData( possiblepositions_ );
	for ( auto ld : possiblepositions_ )
	{
	    const auto& sg2d = Survey::Geometry2D::get( ld->geomID() );
	    zsubsels_.add( ZSubSel(sg2d.zRange()) );
	}
    }
    else
    {
	((CubeData*)(&possiblepositions_))->fillBySI();
	zsubsels_.add( ZSubSel(SI().zRange()) );
    }
}


Seis::Provider::~Provider()
{
    delete selectedpositions_;
    delete &possiblepositions_;
    delete &fetcher();
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


uiRetVal Seis::Provider::setInput( const DBKey& dbky )
{
    Threads::Locker locker( lock_ );
    deleteAndZeroPtr( ioobj_ );
    nrdone_ = 0;

    uiRetVal uirv;
    ioobj_ = getIOObj( dbky );
    if ( !ioobj_ )
    {
	if ( !dbky.isValid() )
            uirv = uiStrings::phrDBIDNotValid();
        else
            uirv = uiStrings::phrCannotFindDBEntry( dbky );
    }
    else
    {
	possiblepositions_.setEmpty();
	getPossiblePositions( uirv );

	if ( !uirv.isOK() )
	    state_ = NeedInput;
	else if ( possiblepositions_.isEmpty() )
            uirv.set( tr("No data in input") );
	else
	    state_ = NeedPrep;

	handleNewPositions();
    }

    return uirv;
}


void Seis::Provider::getPossiblePositions( uiRetVal& uirv ) const
{
    fetcher().getPossiblePositions();
    uirv = fetcher().uirv_;
}


void Seis::Provider::prepWork( uiRetVal& uirv ) const
{
    Fetcher& ftch = mNonConst( fetcher() );
    ftch.reset();
    ftch.prepWork();
    uirv = ftch.uirv_;
}


const SeisTrcTranslator* Seis::Provider::curTransl() const
{
    return fetcher().curTransl();
}


void Seis::Provider::handleNewPositions()
{
    if ( !selectedpositions_ || selectedpositions_->isEmpty() )
    {
	LineCollPos lcp; lcp.toStart();
	if ( is2D() )
	    as2D()->trcpos_ = possiblepositions_.bin2D( lcp );
	else
	    as3D()->trcpos_ = possiblepositions_.binID( lcp );
	totalnr_ = possiblepositions_.totalSize();
    }
    else
    {
	const SPos spos( 0, 0 );
	if ( is2D() )
	    as2D()->trcpos_ = selectedpositions_->getBin2D( spos );
	else
	    as3D()->trcpos_ = selectedpositions_->getBinID( spos );
	totalnr_ = selectedpositions_->totalSize();
    }
}


const DBKey& Seis::Provider::dbKey() const
{
    return ioobj_ ? ioobj_->key() : DBKey::getInvalid();
}


BufferString Seis::Provider::name() const
{
    if ( ioobj_ )
	return ioobj_->getName();
    return BufferString();
}


Pos::GeomID Seis::Provider::geomID( idx_type lidx ) const
{
    if ( !selectedpositions_ )
	return GeomID( possiblepositions_.get(lidx)->linenr_ )
    else if ( !is2D() )
	return GeomID::get3D();

    return GeomID( selectedpositions_->data().firstAtIdx(lidx) );
}


void Seis::Provider::getCurPosition( TrcKey& tk ) const
{
    if ( is2D() )
	tk.setPos( as2D()->curBin2D() );
    else
	tk.setPos( as3D()->curBinID() );
}


bool Seis::Provider::isPresent( const TrcKey& tk ) const
{
    return is2D() ? as2D()->isPresent( tk.bin2D() )
		  : as3D()->isPresent( tk.binID() );
}


void Seis::Provider::getComponentInfo( BufferStringSet& nms,
					   DataType* pdt ) const
{
    nms.setEmpty(); DataType dtype;
    fetcher().getComponentInfo( nms, dtype );
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
    for ( int idx=0; idx<selectedcomponents_.size(); idx++ )
	if ( selectedcomponents_[idx] >= 0 )
	    return true;
    return false;
}


const Pos::ZSubSel& Seis::Provider::zSubSel( idx_type idx ) const
{
    return geomSubSel( idx ).zSubSel();
}


const Survey::GeomSubSel& Seis::Provider::geomSubSel( idx_type idx ) const
{
    if ( is2D() )
	return as2D()->lineSubSel( idx );
    return as3D()->cubeSubSel();
}


void Seis::Provider::setSelData( const SelData& sd ) const
{
    Threads::Locker locker( lock_ );

    delete selectedpositions_;
    selectedpositions_ = sd.applyTo( possiblepositions_ );
    handleNewPositions();

    zsubsels_.setEmpty();
    const auto nrgeomids = sd.nrGeomIDs();
    for ( int idx=0; idx<nrgeomids; idx++ )
	zsubsels_.add( sd.zRange(idx) );

    reportSetupChg();
}


void Seis::Provider::selectComponent( int icomp )
{
    Threads::Locker locker( lock_ );
    const auto curnrsel = selectedcomponents_.size();
    if ( curnrsel == 1 && selectedcomponents_[0] == icomp )
	return;

    selectedcomponents_.setEmpty();
    selectedcomponents_ += icomp;
    reportSetupChg();
}


void Seis::Provider::selectComponents( const TypeSet<int>& comps )
{
    Threads::Locker locker( lock_ );
    if ( comps == selectedcomponents_ )
	return;

    selectedcomponents_ = comps;
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
    mSetParIf( !selectedcomponents_.isEmpty(), sKey::Component(mPlural),
			selectedcomponents_ );
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
    iop.get( sKey::Component(mPlural), selectedcomponents_ );
    int rm = readmode_;
    iop.get( sKey::Mode(), rm );
    readmode_ = (Seis::ReadMode)rm;
    TrcKey trcky;
    if ( iop.get(sKey::Position(),trcky) )
	doGoTo( trcky );

    reportSetupChg();
    return uirv;
}


void Seis::Provider::reportSetupChg()
{
    if ( state_ == Active )
	state_ = NeedPrep;
}


void Seis::Provider::ensureSelectedPositions() const
{
    if ( !selectedpositions_ )
    {
	selectedpositions_ = new BinnedValueSet( possiblepositions_,
                        is2D() ? OD::LineBasedGeom : OD::VolBasedGeom );
	handleNewPositions();
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
	ensureSelectedPositions();
	prepWork( uirv );
	if ( uirv.isOK() )
	    state_ = Active;
    }

    return uirv.isOK();
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
	if ( forcefpdata_ )
	    for ( int idx=0; idx<tbuf.size(); idx++ )
		tbuf.get(idx)->data().convertToFPs();
	nrdone_++;
    }
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


bool Seis::Provider::goTo( const TrcKey& tk ) const
{
    if ( is2D() )
	return as2D()->goTo( tk.bin2D() );
    else
	return as3D()->goTo( tk.binID() );
}


bool Seis::Provider::doGoTo( const TrcKey& tk ) const
{
    if ( state_ != Active )
	return false;

    const SPos newpos( is2D() ? selectedpositions_->find( tk.bin2D() )
			      : selectedpositions_->find( tk.binID() ) );
    if ( !newpos.isValid() )
	return false;

    spos_ = newpos;
    return true;
}


bool Seis::Provider::prepGet( uiRetVal& uirv, bool next ) const
{
    if ( !spos_.isValid() )
	next = true;

    if ( !prepareAccess(uirv) )
	return false;

    if ( next && !selectedpositions_->toNext(spos_) )
	{ uirv.set( uiStrings::sFinished() ); return false; }

    if ( is2D() )
	as2D()->trcpos_ = selectedpositions_->getBin2D();
    else
	as3D()->trcpos_ = selectedpositions_->getBinID();

    return true;
}


uiRetVal Seis::Provider::getCurrent( SeisTrc& trc ) const
{
    Threads::Locker locker( lock_ );
    return doGetTrc( trc, false );
}


uiRetVal Seis::Provider::getNext( SeisTrc& trc ) const
{
    Threads::Locker locker( lock_ );
    return doGetTrc( trc, true );
}


uiRetVal Seis::Provider::doGetTrc( SeisTrc& trc, bool next ) const
{
    uiRetVal uirv;
    if ( !prepGet(uirv,next) )
	return uirv;

    if ( !isPS() )
    {
	gtTrc( trc.data(), trc.info(), uirv );
	locker.unLockNow();
    }
    else
    {
	SeisTrcBuf tbuf( true );
	gtGather( tbuf, uirv );
	locker.unLockNow();
	putGatherInTrace( tbuf, trc );
    }

    wrapUpGet( trc, uirv );
    return uirv;
}


uiRetVal Seis::Provider::getCurrentGather( SeisTrcBuf& tbuf ) const
{
    Threads::Locker locker( lock_ );
    return doGetGath( tbuf, false );
}


uiRetVal Seis::Provider::getNextGather( SeisTrcBuf& tbuf ) const
{
    Threads::Locker locker( lock_ );
    return doGetGath( tbuf, true );
}


uiRetVal Seis::Provider::doGetGath( SeisTrcBuf& tbuf, bool next ) const
{
    uiRetVal uirv;
    if ( !prepGet(uirv,next) )
	return uirv;

    if ( isPS() )
    {
	gtGather( tbuf, uirv );
	locker.unlockNow();
    }
    else
    {
	SeisTrc trc;
	gtTrc( trc.data(), trc.info(), uirv );
	locker.unlockNow();
	if ( uirv.isOK() )
	    putTraceInGather( trc, tbuf );
    }

    wrapUpGet( tbuf, uirv );
    return uirv;
}


uiRetVal Seis::Provider::getAt( const TrcKey& trcky, SeisTrc& trc ) const
{
    Threads::Locker locker( lock_ );
    if ( !doGoTo(trcky) )
	return uiStrings::sNotPresent();
    return doGetTrc( trc, false );
}


uiRetVal Seis::Provider::getGatherAt( const TrcKey& trcky,
				      SeisTrcBuf& tbuf ) const
{
    Threads::Locker locker( lock_ );
    if ( !doGoTo(trcky) )
	return uiStrings::sNotPresent();
    return doGetGather( tbuf, false );
}


uiRetVal Seis::Provider::getNextSequence( Seis::RawTrcsSequence& rawseq ) const
{
    uiRetVal uirv;
    Threads::Locker locker( lock_ );

    if ( !prepGet(uirv,false) )
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
	{ uirv = mINTERNAL("Provider type != seq type"); return; }

    SeisTrc trc;
    SeisTrcInfo& trcinfo = trc.info();
    SeisTrcBuf tbuf( true );
    for ( int ipos=0; ipos<rawseq.nrpos_; ipos++ )
    {
	const TrcKey& tk = (*rawseq.tks_)[ipos];
	if ( !doGoTo(tk) )
	    continue;

	if ( isps )
	{
	    gtGather( tbuf, uirv );
	    if ( !uirv.isOK() )
		return;
	    rawseq.copyFrom( tbuf );
	}
	else
	{
	    gtTrc( *rawseq.data_.get(ipos), trcinfo, uirv );
	    if ( !uirv.isOK() )
		return;

	    const auto* trl = curTransl();
	    if ( trl )
		rawseq.setTrcScaler( ipos, trl->curtrcscale_ );
	}
    }
}


Seis::Provider3D::Provider3D()
    : Provider(false)
{
    handleNewPositions(); // cannot do this in base class
}


void Seis::Provider3D::getCubeData( CubeData& cd ) const
{
    if ( !selectedpositions_ )
	cd = (const CubeData&)possiblepositions_;
    else
    {
	LineCollDataFiller filler( cd );
	SPos spos;
	while ( selectedpositions_->toNext(spos,false) )
	    filler.add( selectedpositions_->getBinID(spos) );
    }
}


bool Seis::Provider3D::isPresent( const BinID& bid ) const
{
    return selectedpositions_ ? selectedpositions_->includes( bid )
			      : possiblepositions_.includes( bid );
}


bool Seis::Provider3D::goTo( const BinID& bid ) const
{
    Threads::Locker locker( lock_ );
    return doGoTo( TrcKey(bid) );
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



Seis::Provider2D::Provider2D()
    : Provider(true)
{
    handleNewPositions(); // cannot do this in base class
}


bool Seis::Provider2D::isPresent( GeomID gid ) const
{
    const auto lnr = gid.lineNr();
    return selectedpositions_ ? selectedpositions_->hasFirst( lnr )
			      : possiblepositions_.includes( lnr );
}


bool Seis::Provider2D::isPresent( const Bin2D& b2d ) const
{
    return selectedpositions_ ? selectedpositions_->includes( b2d )
			      : possiblepositions_.includes( b2d );
}


bool Seis::Provider2D::indexOf( GeomID gid ) const
{
    const auto lnr = gid.lineNr();
    return selectedpositions_ ? selectedpositions_->data().firstIdx( lnr )
			      : possiblepositions_.lineIndexOf( lnr );
}


Seis::Provider::size_type Seis::Provider2D::nrLines() const
{
    return selectedpositions_ ? selectedpositions_->nrFirst()
			      : possiblepositions_.size();
}


Pos::GeomID Seis::Provider2D::geomID( idx_type lidx ) const
{
    auto lnr = selectedpositions_ ? selectedpositions_->data().firstAtIdx(lidx)
				  : possiblepositions_.get(lidx)->linenr_;
}


void Seis::Provider2D::getLineData( idx_type iln, LineData& ld ) const
{
    if ( !selectedpositions_ )
	ld = *possiblepositions_.get( lidx );
    else
    {
	PosInfo::LineDataFiller ldf( ld );
	TypeSet<pos_type> trcnrs;
	selectedpositions_->getSecondsAtIndex( iln, trcnrs );
	PosInfo::LineDataFiller( ld ).add( trcnrs );
    }
}


bool Seis::Provider2D::goTo( const Bin2D& b2d ) const
{
    Threads::Locker locker( lock_ );
    return doGoTo( TrcKey(b2d) );
}


uiRetVal Seis::Provider2D::getAt( const Bin2D& b2d, SeisTrc& trc ) const
{
    return Provider::getAt( TrcKey(b2d), trc );
}


uiRetVal Seis::Provider2D::getGatherAt( const Bin2D& b2d,
					SeisTrcBuf& tbuf ) const
{
    return Provider::getGatherAt( TrcKey(b2d), tbuf );
}
