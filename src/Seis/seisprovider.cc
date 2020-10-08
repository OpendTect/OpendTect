/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/


#include "seisprovider.h"
#include "seisproviderimpl.h"
#include "seisfetcher.h"

#include "binnedvalueset.h"
#include "cubedata.h"
#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "linesdata.h"
#include "posinfo2d.h"
#include "scaler.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisrawtrcsseq.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "trckey.h"
#include "uistrings.h"

static const Seis::Provider::z_type zeps = 1.e-6f;


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
    SeisIOObjInfo objinf( ioobj );
    Provider* ret = 0;
    if ( objinf.isOK() )
    {
	ret = create( objinf.geomType() );
	uiRetVal dum; if ( !uirv ) uirv = &dum;
	*uirv = ret->setInput( ioobj );
	if ( !uirv->isOK() )
	{ delete ret; ret = 0; }
    }

    return ret;
}


Seis::Provider* Seis::Provider::create( const DBKey& dbky, uiRetVal* uirv )
{
    PtrMan<IOObj> ioobj = getIOObj( dbky );

    return ioobj ? create( *ioobj, uirv ) : nullptr;
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


Seis::Provider::Provider( bool is2d, bool fillposs )
    : possiblepositions_(is2d ? *(LineCollData*)new PosInfo::LinesData
			      : *(LineCollData*)new PosInfo::SortedCubeData)
{
    if ( !is2d )
    {
	allgeomids_.add( GeomID::get3D() );
	allzsubsels_.add( ZSubSel(SI().zRange()) );
	if ( fillposs )
	    ((PosInfo::CubeData*)(&possiblepositions_))->fillBySI();
    }
    else
    {
	Survey::Geometry2D::getGeomIDs( allgeomids_ );
	for ( auto gid : allgeomids_ )
	{
	    const auto& sg2d = Survey::Geometry2D::get( gid );
	    allzsubsels_.add( ZSubSel(sg2d.zRange()) );
	}

	if ( fillposs )
	    ((PosInfo::LinesData*)(&possiblepositions_))->setToAllLines();
    }
}


Seis::Provider::~Provider()
{
    delete ioobj_;
    delete seldata_;
    delete selectedpositions_;
    delete &possiblepositions_;
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


Seis::Provider::size_type Seis::Provider::nrGeomIDs() const
{
    return is2D() ? as2D()->nrLines() : 1;
}


void Seis::Provider::ensureLineIdxOK( idx_type& iln ) const
{
    if ( !is2D() )
    {
	if ( iln != 0 )
	    { pErrMsg("For 3D, iln shld always be 0"); iln = 0; }
    }
    else
    {
	if ( iln < 0 )
	    { pErrMsg("iln < 0"); iln = 0; }
	else if ( iln >= as2D()->nrLines() )
	    { pErrMsg("iln outside range"); iln = as2D()->nrLines()-1; }
    }
}


Pos::GeomID Seis::Provider::geomID( int iln ) const
{
    ensureLineIdxOK( iln );
    return is2D() ? as2D()->gtGeomID( iln ) : allgeomids_[iln];
}


Pos::ZSubSel& Seis::Provider::possZSubSel( int iln )
{
    ensureLineIdxOK( iln );
    return is2D() ? as2D()->gtPossZSubSel( iln ) : allzsubsels_[iln];
}


const Pos::ZSubSel& Seis::Provider::zSubSel( int iln ) const
{
    ensureLineIdxOK( iln );
    if ( selectedpositions_ )
	return selectedzsubsels_[iln];
    return mSelf().possZSubSel( iln );
}


void Seis::Provider::getFullHorSubSel( FullHorSubSel& fhss, bool target ) const
{
    possiblepositions_.getFullHorSubSel( fhss, is2D() );
    const auto posns = selectedPositions( target );
    if ( posns )
    {
	if ( !is2D() )
	{
	    const auto selinlrg = posns->firstRange();
	    const auto selcrlrg = posns->secondRange();
	    auto inlrg = fhss.inlRange();
	    auto crlrg = fhss.crlRange();
	    inlrg.start = selinlrg.start; inlrg.stop = selinlrg.stop;
	    crlrg.start = selcrlrg.start; crlrg.stop = selcrlrg.stop;
	    fhss.setInlRange( inlrg );
	    fhss.setCrlRange( crlrg );
	}
	else
	{
	    auto& lhsss = fhss.subSel2D();
	    for ( auto iln=0; iln<lhsss.size(); iln++ )
	    {
		auto& lhss = *lhsss.get( iln );
		const auto lineid = lhss.geomID().getI();
		if ( !posns->hasFirst(lineid) )
		    { lhsss.removeSingle( iln ); iln--; }
		else
		{
		    const auto seltnrrg = posns->secondRange( lineid );
		    auto tnrrg = lhss.trcNrRange();
		    tnrrg.start = seltnrrg.start; tnrrg.stop = seltnrrg.stop;
		    lhss.setTrcNrRange( seltnrrg );
		}
	    }
	}
    }
}


void Seis::Provider::getFullZSubSel( FullZSubSel& fzss ) const
{
    fzss.setEmpty();
    const auto nrgeomids = nrGeomIDs();
    for ( auto idx=0; idx<nrgeomids; idx++ )
	fzss.set( geomID(idx), zSubSel(idx) );
}


void Seis::Provider::getFullSubSel( FullSubSel& fss, bool target ) const
{
    getFullHorSubSel( fss.fullHorSubSel(), target );
    getFullZSubSel( fss.fullZSubSel() );
}


const BinnedValueSet* Seis::Provider::selectedPositions( bool target ) const
{
    return target && targetpositions_ ? targetpositions_ : selectedpositions_;
}


uiRetVal Seis::Provider::setInput( const DBKey& dbky )
{
    uiRetVal uirv;
    if ( !dbky.isValid() )
    {
	uirv = uiStrings::phrDBIDNotValid();
	return uirv;
    }
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    if ( !ioobj )
    {
	uirv = uiStrings::phrCannotFindDBEntry( dbky );
	return uirv;
    }
    return setInput( *ioobj );
}


uiRetVal Seis::Provider::setInput( const IOObj& inioobj )
{
    deleteAndZeroPtr( ioobj_ );
    nrdone_ = 0;
    ioobj_ = inioobj.clone();
    uiRetVal uirv;
    possiblepositions_.setEmpty();
    fetcher().getPossibleExtents();
    uirv = fetcher().uirv_;

    if ( !uirv.isOK() )
	state_ = NeedInput;
    else if ( possiblepositions_.isEmpty() )
	uirv.set( tr("No data in input") );
    else
	state_ = NeedPrep;

    handleNewPositions();
    return uirv;
}


void Seis::Provider::prepWork( uiRetVal& uirv )
{
    Fetcher& ftch = fetcher();
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
	PosInfo::LineCollDataPos lcdp; lcdp.toStart();
	if ( is2D() )
	    as2D()->trcpos_ = possiblepositions_.bin2D( lcdp );
	else
	    as3D()->trcpos_ = possiblepositions_.binID( lcdp );
	targettotalnr_ = totalnr_ = possiblepositions_.totalSize();
    }
    else
    {
	const SPos spos( 0, 0 );
	if ( is2D() )
	    as2D()->trcpos_ = selectedpositions_->getBin2D( spos );
	else
	    as3D()->trcpos_ = selectedpositions_->getBinID( spos );
	targettotalnr_ = totalnr_ = selectedpositions_->totalSize();
	if ( targetpositions_ )
	    targettotalnr_ = targetpositions_->totalSize();
    }
}


DBKey Seis::Provider::dbKey() const
{
    return ioobj_ ? ioobj_->key() : DBKey::getInvalid();
}


BufferString Seis::Provider::name() const
{
    if ( ioobj_ )
	return ioobj_->getName();
    return BufferString();
}


bool Seis::Provider::isEmpty() const
{
    return selectedpositions_ ? selectedpositions_->isEmpty()
			      : possiblepositions_.isEmpty();
}


void Seis::Provider::getPositions( LineCollData& lcd, bool target ) const
{
    if ( !selectedpositions_ )
	lcd = possiblepositions_;
    else
    {
	const auto* posns = selectedPositions( target );
	PosInfo::LineCollDataFiller filler( lcd );
	SPos spos;
	while ( posns->next(spos,true) )
	{
	    if ( is2D() )
		filler.add( posns->getBin2D(spos) );
	    else
		filler.add( posns->getBinID(spos) );
	}
    }
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


DataCharacteristics Seis::Provider::trcDataRep() const
{
    return fetcher().dataChar();
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


void Seis::Provider::setSelData( SelData* sd )
{
    delete seldata_;
    seldata_ = sd;
    reportSetupChg();
}


void Seis::Provider::setSelData( const SelData& sd )
{
    delete seldata_;
    seldata_ = sd.clone();
    reportSetupChg();
}


void Seis::Provider::setZRange( const ZSampling& zrg, idx_type iln )
{
    ensureLineIdxOK( iln );
    if ( is2D() )
	as2D()->stZRange( iln, zrg );
    else
	allzsubsels_[iln].setOutputZRange( zrg );

    reportSetupChg();
}


void Seis::Provider::setZExtension( const z_rg_type& zrg )
{
    if ( zrg.isEqual(zextension_,zeps) )
	return;

    zextension_ = zrg;
    reportSetupChg();
}


void Seis::Provider::selectComponent( int icomp )
{
    const auto curnrsel = selectedcomponents_.size();
    if ( curnrsel == 1 && selectedcomponents_[0] == icomp )
	return;

    selectedcomponents_.setEmpty();
    selectedcomponents_ += icomp;
    reportSetupChg();
}


void Seis::Provider::selectComponents( const TypeSet<int>& comps )
{
    if ( comps == selectedcomponents_ )
	return;

    selectedcomponents_ = comps;
    reportSetupChg();
}


void Seis::Provider::setReadMode( ReadMode rm )
{
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
    deleteAndZeroPtr( targetpositions_ );
    deleteAndZeroPtr( selectedpositions_ );
}


od_int64 Seis::Provider::totalNr( bool target ) const
{
    commitSelections();
    return target ? targettotalnr_ : totalnr_;
}


void Seis::Provider::commitSelections() const
{
    if ( selectedpositions_ )
	return;

    auto& self = mSelf();

    if ( !seldata_ )
	self.selectedpositions_ = new BinnedValueSet( possiblepositions_,
			is2D() ? OD::LineBasedGeom : OD::VolBasedGeom );
    else
    {
	self.getSelectedPositionsFromSelData();
	self.applyStepout();
    }

    self.createSelectedZSubSels();

    self.handleNewPositions();
}


void Seis::Provider::getSelectedPositionsFromSelData()
{
    if ( !selectedpositions_ )
	selectedpositions_ = new BinnedValueSet( 0, false,
		is2D() ? OD::LineBasedGeom : OD::VolBasedGeom );

    PosInfo::LineCollDataIterator lcdit( possiblepositions_ );
    if ( is2D() )
    {
	Bin2D b2d;
	while ( lcdit.next(b2d) )
	    if ( seldata_->isOK(b2d) )
		selectedpositions_->add( b2d );
    }
    else
    {
	BinID bid;
	while ( lcdit.next(bid) )
	    if ( seldata_->isOK(bid) )
		selectedpositions_->add( bid );
    }
}


void Seis::Provider::applyStepout()
{
    TypeSet<SPos> toremove;
    SPos spos;
    deleteAndZeroPtr( targetpositions_ );

    if ( is2D() )
    {
	const auto so = as2D()->stepout_;
	if ( so > 0 )
	{
	    targetpositions_ = new BinnedValueSet( *selectedpositions_ );
	    selectedpositions_->setStepout( as2D()->stepout_ );
	    while ( selectedpositions_->next(spos) )
		if ( !possiblepositions_.includes(
				    selectedpositions_->getBin2D(spos)) )
		    toremove += spos;
	}
    }
    else
    {
	const auto& as3d = *as3D();
	const auto so = as3d.stepout_;
	if ( so.inl() > 0 || so.crl() > 0 )
	{
	    targetpositions_ = new BinnedValueSet( *selectedpositions_ );
	    selectedpositions_->setStepout( as3d.stepout_, as3d.binIDStep() );
	    while ( selectedpositions_->next(spos) )
		if ( !possiblepositions_.includes(
				    selectedpositions_->getBinID(spos)) )
		    toremove += spos;
	}
    }

    selectedpositions_->remove( toremove );
}


void Seis::Provider::createSelectedZSubSels()
{
    selectedzsubsels_.setEmpty();

    if ( !is2D() )
    {
	selectedzsubsels_.add( allzsubsels_.first() );
	if ( seldata_ )
	{
	    if ( seldata_->isRange() )
		selectedzsubsels_.last().limitTo(
				seldata_->asRange()->zSubSel(0) );
	    else
		selectedzsubsels_.last().limitTo( seldata_->zRange(0) );
	}
    }
    else
    {
	const auto& prov2d = *as2D();
	const auto nrlines = prov2d.nrLines();
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    const auto geomid = geomID( idx );
	    const auto zssidx = allgeomids_.indexOf( geomid );
	    selectedzsubsels_.add( allzsubsels_.get(zssidx) );
	    if ( seldata_ )
	    {
		const auto sdidx = seldata_->indexOf( geomid );
		if ( sdidx < 0 )
		    { pErrMsg("Huh"); continue; }
		selectedzsubsels_.last().limitTo( seldata_->zRange(sdidx) );
	    }
	}
    }

    for ( auto& zss : selectedzsubsels_ )
	zss.zData().widen( zextension_ );
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
	commitSelections();
	mSelf().prepWork( uirv );
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
    {
	uiRetVal uirv;
	if ( !prepareAccess(uirv) )
	    return false;
    }

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

    if ( next && !selectedpositions_->next(spos_,true) )
	{ uirv.set( uiStrings::sFinished() ); return false; }

    setTrcPos();
    return true;
}


void Seis::Provider::setTrcPos() const
{
    if ( is2D() )
	mSelf().as2D()->trcpos_ = selectedpositions_->getBin2D(spos_);
    else
	mSelf().as3D()->trcpos_ = selectedpositions_->getBinID(spos_);
}


uiRetVal Seis::Provider::getCurrent( SeisTrc& trc ) const
{
    Threads::Locker locker( getlock_ );
    return doGetTrc( trc, false );
}


uiRetVal Seis::Provider::getNext( SeisTrc& trc ) const
{
    Threads::Locker locker( getlock_ );
    return doGetTrc( trc, true );
}


uiRetVal Seis::Provider::doGetTrc( SeisTrc& trc, bool next ) const
{
    uiRetVal uirv;
    if ( !prepGet(uirv,next) )
	return uirv;

    if ( !isPS() )
	gtTrc( trc.data(), trc.info(), uirv );
    else
    {
	SeisTrcBuf tbuf( true );
	gtGather( tbuf, uirv );
	putGatherInTrace( tbuf, trc );
    }

    wrapUpGet( trc, uirv );
    return uirv;
}


uiRetVal Seis::Provider::getCurrentGather( SeisTrcBuf& tbuf ) const
{
    Threads::Locker locker( getlock_ );
    return doGetGath( tbuf, false );
}


uiRetVal Seis::Provider::getNextGather( SeisTrcBuf& tbuf ) const
{
    Threads::Locker locker( getlock_ );
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
    }
    else
    {
	SeisTrc trc;
	gtTrc( trc.data(), trc.info(), uirv );
	if ( uirv.isOK() )
	    putTraceInGather( trc, tbuf );
    }

    wrapUpGet( tbuf, uirv );
    return uirv;
}


uiRetVal Seis::Provider::getAt( const TrcKey& trcky, SeisTrc& trc ) const
{
    Threads::Locker locker( getlock_ );
    if ( !doGoTo(trcky) )
	return uiStrings::sNotPresent();
    return doGetTrc( trc, false );
}


uiRetVal Seis::Provider::getGatherAt( const TrcKey& trcky,
				      SeisTrcBuf& tbuf ) const
{
    Threads::Locker locker( getlock_ );
    if ( !doGoTo(trcky) )
	return uiStrings::sNotPresent();
    return doGetGath( tbuf, false );
}


uiRetVal Seis::Provider::getNextSequence( Seis::RawTrcsSequence& rawseq ) const
{
    uiRetVal uirv;
    Threads::Locker locker( getlock_ );

    if ( !prepGet(uirv,false) )
	return uirv;

    fillSequence( rawseq, uirv );
    nrdone_ += rawseq.nrPositions();

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

	setTrcPos();

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
    : Provider(false,false)
{
    handleNewPositions(); // cannot do this in base class
}


Seis::Provider3D::Provider3D( const DBKey& dbky, uiRetVal& uirv )
    : Provider(false,true)
{
    uirv = setInput( dbky );
}


void Seis::Provider3D::setStepout( const IdxPair& so )
{
    if ( stepout_ != so )
	{ stepout_ = so; reportSetupChg(); }
}


const PosInfo::CubeData& Seis::Provider3D::possibleCubeData() const
{
    return (const CubeData&)possiblepositions_;
}


bool Seis::Provider3D::isPresent( const BinID& bid ) const
{
    return selectedpositions_ ? selectedpositions_->includes( bid )
			      : possiblepositions_.includes( bid );
}


BinID Seis::Provider3D::binIDStep() const
{
    return possibleCubeData().minStep();
}


bool Seis::Provider3D::goTo( const BinID& bid ) const
{
    Threads::Locker locker( getlock_ );
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


Seis::Provider3D& Seis::Provider3D::dummy()
{
    static auto* ret = new VolProvider;
    ret->possiblepositions_.setEmpty();
    return *ret;
}
const Seis::Provider3D& Seis::Provider3D::empty()
{
    return dummy();
}



Seis::Provider2D::Provider2D()
    : Provider(true,true)
{
    handleNewPositions(); // cannot do this in base class
}


Seis::Provider2D::Provider2D( const DBKey& dbky, uiRetVal& uirv )
    : Provider(true,false)
{
    uirv = setInput( dbky );
}


void Seis::Provider2D::setStepout( trcnr_type so )
{
    if ( stepout_ != so )
	{ stepout_ = so; reportSetupChg(); }
}


const PosInfo::LinesData& Seis::Provider2D::possibleLinesData() const
{
    return (const LinesData&)possiblepositions_;
}


bool Seis::Provider2D::isPresent( GeomID gid ) const
{
    const auto lnr = gid.lineNr();
    return selectedpositions_ ? selectedpositions_->hasFirst( lnr )
			      : possiblepositions_.includesLine( lnr );
}


bool Seis::Provider2D::isPresent( const Bin2D& b2d ) const
{
    return selectedpositions_ ? selectedpositions_->includes( b2d )
			      : possiblepositions_.includes( b2d );
}


Seis::Provider::size_type Seis::Provider2D::nrLines() const
{
    return selectedpositions_ ? selectedpositions_->nrFirst()
			      : possiblepositions_.size();
}


Seis::Provider2D::idx_type Seis::Provider2D::lineIdx( GeomID gid ) const
{
    const auto lnr = gid.lineNr();
    return selectedpositions_ ? selectedpositions_->data().firstIdx( lnr )
			      : possiblepositions_.lineIndexOf( lnr );
}


void Seis::Provider2D::getLineData( idx_type iln, LineData& ld ) const
{
    ensureLineIdxOK( iln );
    if ( !selectedpositions_ )
	ld = *possiblepositions_.get( iln );
    else
    {
	PosInfo::LineDataFiller ldf( ld );
	TypeSet<trcnr_type> trcnrs;
	selectedpositions_->data().getSecondsAtIndex( iln, trcnrs );
	PosInfo::LineDataFiller( ld ).add( trcnrs );
    }
}


Seis::Provider2D::trcnr_type Seis::Provider2D::trcNrStep( idx_type iln ) const
{
    if ( iln < 0 )
	return 1;
    LineData ld;
    getLineData( iln, ld );
    return ld.minStep();
}


bool Seis::Provider2D::goTo( const Bin2D& b2d ) const
{
    Threads::Locker locker( getlock_ );
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


Seis::Provider2D& Seis::Provider2D::dummy()
{
    static auto* ret = new LineProvider;
    ret->possiblepositions_.setEmpty();
    return *ret;
}
const Seis::Provider2D& Seis::Provider2D::empty()
{
    return dummy();
}



Pos::GeomID Seis::Provider2D::gtGeomID( idx_type lidx ) const
{
    if ( selectedpositions_ )
	return GeomID( selectedpositions_->data().firstAtIdx(lidx) );

    return GeomID( possiblepositions_[lidx]->linenr_ );
}


Pos::ZSubSel& Seis::Provider2D::gtPossZSubSel( idx_type iln ) const
{
    ensureLineIdxOK( iln );
    const auto gid = gtGeomID( iln );
    const auto zsidx = allgeomids_.indexOf( gid );
    return mNonConst( allzsubsels_[zsidx] );
}


void Seis::Provider2D::stZRange( idx_type iln, const ZSampling& zrg )
{
    ensureLineIdxOK( iln );
    const auto gid = gtGeomID( iln );
    const auto zsidx = allgeomids_.indexOf( gid );
    allzsubsels_[zsidx].setOutputZRange( zrg );
}
