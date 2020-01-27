/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2019
-*/


#include "fullsubsel.h"
#include "cubesubsel.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"


mUseType( Survey::FullHorSubSel, pos_type );
mUseType( Survey::FullHorSubSel, pos_rg_type );
mUseType( Survey::FullHorSubSel, idx_type );
mUseType( Survey::FullHorSubSel, size_type );
typedef StepInterval<pos_type> pos_steprg_type;


const char* Survey::FullHorSubSel::sNrLinesKey()
{
    return IOPar::compKey( sKey::Line(mPlural),sKey::Size() );
}


Survey::FullHorSubSel::FullHorSubSel( const SurveyInfo* si )
    : chss_(new CubeHorSubSel(si))
    , lhsss_(*new LineHorSubSelSet)
{
}


Survey::FullHorSubSel::FullHorSubSel( GeomID gid, const SurveyInfo* si )
    : lhsss_(*new LineHorSubSelSet)
{
    if ( gid.is2D() )
	lhsss_ += new LineHorSubSel( gid );
    else
	chss_ = new CubeHorSubSel( si );
}


Survey::FullHorSubSel::FullHorSubSel( const BinID& bid )
    : FullHorSubSel( TrcKey(bid) )
{
}


Survey::FullHorSubSel::FullHorSubSel( GeomID gid, trcnr_type tnr )
    : FullHorSubSel( TrcKey(gid,tnr) )
{
}


Survey::FullHorSubSel::FullHorSubSel( const TrcKey& tk )
    : lhsss_(*new LineHorSubSelSet)
{
    if ( tk.is2D() )
	lhsss_ += new LineHorSubSel( tk.geomID(), tk.trcNr() );
    else
	chss_ = new CubeHorSubSel( tk.binID() );
}


Survey::FullHorSubSel::FullHorSubSel( const GeomIDSet& gids,
				      const SurveyInfo* si )
    : lhsss_(*new LineHorSubSelSet)
{
    if ( !gids.isEmpty() && !gids.first().is2D() )
	chss_ = new CubeHorSubSel( si );
    else
	for ( auto gid : gids )
	    lhsss_ += new LineHorSubSel( gid );
}


Survey::FullHorSubSel::FullHorSubSel( const CubeHorSubSel& chss )
    : chss_(new CubeHorSubSel(chss))
    , lhsss_(*new LineHorSubSelSet)
{
}


Survey::FullHorSubSel::FullHorSubSel( const LineHorSubSel& lhss )
    : lhsss_(*new LineHorSubSelSet)
{
    lhsss_ += new LineHorSubSel( lhss );
}


Survey::FullHorSubSel::FullHorSubSel( const GeomSubSel& gss )
    : lhsss_(*new LineHorSubSelSet)
{
    if ( gss.is2D() )
	lhsss_ += new LineHorSubSel( gss.asLineSubSel()->lineHorSubSel() );
    else
	chss_ = new CubeHorSubSel( gss.asCubeSubSel()->cubeHorSubSel() );
}


Survey::FullHorSubSel::FullHorSubSel( const CubeSubSel& css )
    : chss_(new CubeHorSubSel(css.cubeHorSubSel()))
    , lhsss_(*new LineHorSubSelSet)
{
}


Survey::FullHorSubSel::FullHorSubSel( const LineSubSel& lss )
    : lhsss_(*new LineHorSubSelSet)
{
    lhsss_ += new LineHorSubSel( lss.lineHorSubSel() );
}


Survey::FullHorSubSel::FullHorSubSel( const LineHorSubSelSet& lhsss )
    : lhsss_(*new LineHorSubSelSet)
{
    deepCopy( lhsss_, lhsss );
}


Survey::FullHorSubSel::FullHorSubSel( const LineSubSelSet& lsss )
    : lhsss_(*new LineHorSubSelSet)
{
    for ( auto lss : lsss )
	lhsss_ += new LineHorSubSel( lss->lineHorSubSel() );
}


Survey::FullHorSubSel::FullHorSubSel( const FullSubSel& fss )
    : FullHorSubSel(fss.fullHorSubSel())
{
}


Survey::FullHorSubSel::FullHorSubSel( const TrcKeySampling& tks )
    : lhsss_(*new LineHorSubSelSet)
{
    if ( tks.is2D() )
	lhsss_ += new LineHorSubSel( tks );
    else
	chss_ = new CubeHorSubSel( tks );
}


Survey::FullHorSubSel::FullHorSubSel( const TrcKeyZSampling& tkzs )
    : lhsss_(*new LineHorSubSelSet)
{
    if ( tkzs.is2D() )
	lhsss_ += new LineHorSubSel( tkzs.hsamp_ );
    else
	chss_ = new CubeHorSubSel( tkzs.hsamp_ );
}


Survey::FullHorSubSel::FullHorSubSel( const FullHorSubSel& oth )
    : lhsss_(*new LineHorSubSelSet)
{
    *this = oth;
}


Survey::FullHorSubSel::FullHorSubSel( const IOPar& iop, const SurveyInfo* si )
    : FullHorSubSel()
{
    usePar( iop, si );
}


Survey::FullHorSubSel::~FullHorSubSel()
{
    clearContents();
    delete chss_;
    delete &lhsss_;
}


Survey::FullHorSubSel& Survey::FullHorSubSel::operator=(
						const FullHorSubSel& oth )
{
    if ( this != &oth )
    {
	clearContents();
	if ( oth.chss_ )
	{
	    delete chss_;
	    chss_ = new CubeHorSubSel( *oth.chss_ );
	}
	else
	    lhsss_ = oth.lhsss_;
    }
    return *this;
}


bool Survey::FullHorSubSel::operator==( const FullHorSubSel& oth ) const
{
    if ( is2D() != oth.is2D() )
	return false;

    if ( oth.chss_ )
	return oth.chss_->equals( *chss_ );
    else
	return lhsss_ == oth.lhsss_;
}


const SurveyInfo& Survey::FullHorSubSel::survInfo() const
{
    if ( chss_ )
	return chss_->survInfo();
    return SI();
}


void Survey::FullHorSubSel::clearContents()
{
    deleteAndZeroPtr( chss_ );
    lhsss_.setEmpty();
}


void Survey::FullHorSubSel::set3D( bool yn, const SurveyInfo* si )
{
    if ( !yn )
	deleteAndZeroPtr( chss_ );
    else
    {
	lhsss_.setEmpty();
	if ( !chss_ )
	{
	    delete chss_;
	    chss_ = new CubeHorSubSel( si );
	}
    }
}


void Survey::FullHorSubSel::addGeomID( GeomID gid )
{
    set3D( !gid.is2D() );
    if ( is2D() )
    {
	const auto idxof = indexOf( gid );
	if ( idxof < 0 )
	    lhsss_.add( new LineHorSubSel( gid ) );
    }
}


void Survey::FullHorSubSel::setGeomID( GeomID gid, const SurveyInfo* si )
{
    if ( gid.is3D() )
    {
	lhsss_.setEmpty();
	if ( chss_ && &SI(si) != &chss_->survInfo() )
	{
	    delete chss_;
	    chss_ = new CubeHorSubSel( si );
	}
    }
    else
    {
	if ( lhsss_.size() == 1 && lhsss_.get(0)->geomID() == gid )
	    return;

	lhsss_.setEmpty();
	addGeomID( gid );
    }
}


void Survey::FullHorSubSel::merge( const FullHorSubSel& oth )
{
    if ( oth.is2D() != is2D() )
	{ pErrMsg("2D/3D mismatch"); return; }
    if ( oth.chss_ )
	chss_->merge( *oth.chss_ );
    else
	lhsss_.merge( oth.lhsss_ );
}


void Survey::FullHorSubSel::limitTo( const FullHorSubSel& oth )
{
    if ( oth.is2D() != is2D() )
	{ pErrMsg("2D/3D mismatch"); return; }
    if ( oth.chss_ )
	chss_->limitTo( *oth.chss_ );
    else
	lhsss_.limitTo( oth.lhsss_ );
}


bool Survey::FullHorSubSel::isAll() const
{
    if ( chss_ )
	return chss_->isAll();
    else
	return lhsss_.isAll();
}


static pos_type getInlCrl42D( const SurvGeom2D& geom,
			      bool first, bool inl, const SurveyInfo& si )
{
    const Coord coord( geom.getCoordByIdx(first? 0 : geom.size()-1) );
    const BinID bid( si.transform(coord) );
    return inl ? bid.inl() : bid.crl();
}


static void inclInlCrl42D( const SurvGeom2D& geom,
			   pos_rg_type& rg, bool inl, const SurveyInfo& si )
{
    if ( geom.isEmpty() )
	return;

    auto posidx = getInlCrl42D( geom, true, inl, si );
    if ( mIsUdf(rg.start) )
	rg.start = rg.stop = posidx;
    else
	rg.include( posidx, false );
    posidx = getInlCrl42D( geom, false, inl, si );
    rg.include( posidx );
}


pos_steprg_type Survey::FullHorSubSel::inlRange() const
{
    if ( chss_ )
	return chss_->inlRange();

    pos_rg_type ret( mUdf(int), mUdf(int) );
    for ( auto idx=0; idx<nrGeomIDs(); idx++ )
	inclInlCrl42D( SurvGeom2D::get( geomID(idx) ), ret, true, survInfo() );

    return pos_steprg_type( ret, survInfo().inlStep() );
}


pos_steprg_type Survey::FullHorSubSel::crlRange() const
{
    if ( chss_ )
	return chss_->crlRange();

    pos_rg_type ret( mUdf(int), mUdf(int) );
    for ( auto idx=0; idx<nrGeomIDs(); idx++ )
	inclInlCrl42D( SurvGeom2D::get( geomID(idx) ), ret, false, survInfo() );

    return pos_steprg_type( ret, survInfo().crlStep() );
}



pos_steprg_type Survey::FullHorSubSel::trcNrRange( idx_type iln ) const
{
    if ( chss_ )
	return crlRange();
    else if ( iln >= lhsss_.size() )
	{ pErrMsg("iln>=sz"); return pos_rg_type(); }

    return lhsss_.get(iln)->trcNrRange();
}


size_type Survey::FullHorSubSel::nrGeomIDs() const
{
    return chss_ ? 1 : lhsss_.size();
}


Pos::GeomID Survey::FullHorSubSel::geomID( idx_type iln ) const
{
    if ( chss_ )
	return GeomID::get3D();
    else if ( iln >= lhsss_.size() )
	return GeomID();
    return lhsss_.get(iln)->geomID();
}


idx_type Survey::FullHorSubSel::indexOf( GeomID gid ) const
{
    if ( !is2D() )
	return gid.is2D() ? -1 : 0;
    for ( int idx=0; idx<lhsss_.size(); idx++ )
	if ( lhsss_.get(idx)->geomID() == gid )
	    return idx;
    return -1;
}


void Survey::FullHorSubSel::setInlRange( const pos_rg_type& rg )
{
    if ( !chss_ )
	return;
    if ( rg.hasStep() )
	chss_->setInlRange( (const pos_steprg_type&)rg );
    else
	chss_->setInlRange( pos_steprg_type(rg,survInfo().inlStep()) );
}


void Survey::FullHorSubSel::setCrlRange( const pos_rg_type& rg )
{
    if ( !chss_ )
	return;
    if ( rg.hasStep() )
	chss_->setCrlRange( (const pos_steprg_type&)rg );
    else
	chss_->setCrlRange( pos_steprg_type(rg,survInfo().crlStep()) );
}


void Survey::FullHorSubSel::setTrcNrRange( GeomID gid, const pos_rg_type& rg )
{
    const auto idxof = indexOf( gid );
    if ( idxof >= 0 )
	setTrcNrRange( rg, idxof );
}


void Survey::FullHorSubSel::setTrcNrRange( const pos_rg_type& rg, idx_type idx )
{
    if ( !is2D() )
	setCrlRange( rg );
    else if ( idx >= lhsss_.size() )
	{ pErrMsg("idx bad"); }
    else
    {
	auto& lhss = *lhsss_[idx];
	if ( rg.hasStep() )
	    lhss.setTrcNrRange( (const pos_steprg_type&)rg );
	else
	    lhss.setTrcNrRange( pos_steprg_type(rg,lhss.trcNrRange().step) );
    }
}


void Survey::FullHorSubSel::setEmpty()
{
    clearContents();
}


void Survey::FullHorSubSel::setToAll( bool is2d, const SurveyInfo* si )
{
    clearContents();
    if ( is2d )
	lhsss_.setToAll();
    else
	chss_ = new CubeHorSubSel( si );
}


void Survey::FullHorSubSel::setFull( GeomID gid, const SurveyInfo* si )
{
    if ( !gid.isValid() )
	return;

    set3D( gid.is3D(), si );
    if ( !is2D() )
	{ delete chss_; chss_ = new CubeHorSubSel( si ); return; }

    const auto idxof = indexOf( gid );
    if ( idxof < 0 )
	lhsss_.add( new LineHorSubSel(gid) );
    else
	*lhsss_.get(idxof) = LineHorSubSel( gid );
}


void Survey::FullHorSubSel::set( const CubeHorSubSel& chss )
{
    set3D( true, &chss.survInfo() );
    *chss_ = chss;
}


void Survey::FullHorSubSel::set( const CubeSubSel& css )
{
    set3D( true, &css.survInfo() );
    *chss_ = css.cubeHorSubSel();
}


void Survey::FullHorSubSel::set( const LineHorSubSel& lhss )
{
    set3D( false );
    lhsss_.setEmpty();
    lhsss_.add( new LineHorSubSel(lhss) );
}


void Survey::FullHorSubSel::set( const LineSubSel& lss )
{
    set3D( false );
    lhsss_.setEmpty();
    lhsss_.add( new LineHorSubSel(lss) );
}


void Survey::FullHorSubSel::set( const LineHorSubSelSet& lhsss )
{
    set3D( false );
    lhsss_ = lhsss;
}


void Survey::FullHorSubSel::set( const LineSubSelSet& lsss )
{
    set3D( false );
    for ( auto lss : lsss )
	lhsss_.add( new LineHorSubSel(*lss) );
}


void Survey::FullHorSubSel::fillPar( IOPar& iop ) const
{
    if ( chss_ )
	chss_->fillPar( iop );
    else
    {
	iop.set( sNrLinesKey(), lhsss_.size() );
	for ( int idx=0; idx<lhsss_.size(); idx++ )
	{
	    IOPar subiop;
	    lhsss_.get(idx)->fillPar( subiop );
	    iop.mergeComp( subiop, toString(idx) );
	}
    }
}


void Survey::FullHorSubSel::usePar( const IOPar& iop, const SurveyInfo* si )
{
    bool iopis2d = iop.isPresent( sNrLinesKey() ); //From Survey namespace
    if ( !iopis2d && !iop.isPresent(sKey::GeomSystem()) )
    {
	iopis2d = !iop.isPresent(sKey::SurveyID()); //From Seis::SelData
	if ( iopis2d )
	{
	    BufferString firstky( IOPar::compKey(sKey::Line(),toString(0) ) );
	    firstky.set( IOPar::compKey(firstky.buf(),sKey::GeomID() ) );
	    if ( !iop.isPresent(firstky) )
		return;
	}
    }

    clearContents();
    set3D( !iopis2d, si );

    if ( chss_ )
	chss_->usePar( iop );
    else
    {
	size_type nrlines = 0;
	const bool hasnrlines = iop.get( sNrLinesKey(), nrlines );
	if ( !hasnrlines )
	    nrlines = 999999;
	for ( auto idx=0; idx<nrlines; idx++ )
	{
	    const BufferString subselkey( hasnrlines ? toString(idx)
				: IOPar::compKey(sKey::Line(),toString(idx)) );
	    PtrMan<IOPar> subpar = iop.subselect( subselkey );
	    if ( !subpar || subpar->isEmpty() )
	    {
		if ( hasnrlines )
		    continue;
		else
		    break;
	    }

	    bool is2d = true; GeomID gid;
	    Survey::SubSel::getInfo( *subpar, is2d, gid );
	    if ( is2d && gid.isValid() )
	    {
		auto* lhss = new LineHorSubSel( gid );
		lhss->usePar( *subpar );
		lhsss_ += lhss;
	    }
	}
    }
}


int Survey::FullHorSubSel::selRes3D( const BinID& bid ) const
{
    int ret;
    if ( chss_ )
    {
	const auto inlrg = chss_->inlRange();
	const auto crlrg = chss_->crlRange();
	int inlres = inlrg.start > bid.inl() || inlrg.stop < bid.inl() ? 2 : 0;
	int crlres = crlrg.start > bid.crl() || crlrg.stop < bid.crl() ? 2 : 0;
	ret = inlres + 256 * crlres;
	if ( ret == 0 )
	{
	    inlres = (bid.inl() - inlrg.start) % inlrg.step ? 1 : 0;
	    crlres = (bid.crl() - crlrg.start) % crlrg.step ? 1 : 0;
	    ret = inlres + 256 * crlres;
	}
    }
    else
    {
	const Coord coord = survInfo().transform( bid );
	ret = 2 + 256 * 2;
	for ( int iln=0; iln<lhsss_.size(); iln++ )
	{
	    const auto& lhss = *lhsss_[iln];
	    const auto rg = lhss.trcNrRange();
	    const auto& geom = lhss.geometry2D();
	    auto trcnr = geom.nearestTracePosition( coord );
	    if ( trcnr >= 0 )
	    {
		const BinID trcbid( survInfo().transform(geom.getCoord(trcnr)));
		if ( trcbid == bid )
		    { ret = 0; break; }
	    }
	}
    }

    return ret;
}


int Survey::FullHorSubSel::selRes2D( GeomID gid, pos_type trcnr ) const
{
    int ret;
    const auto lidx = indexOf( gid );
    if ( lidx < 0 )
	ret = 2 + 256 * 2;
    else
    {
	const auto rg = lhsss_[lidx]->trcNrRange();
	int res = rg.start > trcnr || rg.stop < trcnr ? 2 : 0;
	if ( res == 0 )
	    res = (trcnr - rg.start) % rg.step ? 1 : 0;
	ret = 256 * res;
    }
    return ret;
}


uiString Survey::FullHorSubSel::getUserSummary() const
{
    if ( !chss_ )
    {
	if ( lhsss_.size() < 1 )
	    return toUiString( "-" );

	const auto nrtrcs = expectedNrPositions();
	const auto nrlines = lhsss_.size();
	if ( nrlines > 1 )
	    return toUiString("%1: %2 (%3: %4)")
			.arg( uiStrings::sLine(nrlines) ).arg( nrlines )
			.arg( uiStrings::sTrace(nrtrcs) ).arg( nrtrcs );
	const auto& lhss = *lhsss_.first();
	const auto nrrg = lhss.trcNrRange();
	return toUiString( "%1-%2 [%3]" )
		    .arg( nrrg.start ).arg( nrrg.stop )
		    .arg( lhss.geomID().name() );
    }

    const auto& inlss = chss_->inlSubSel(); const auto inlsz = inlss.size();
    const auto& crlss = chss_->crlSubSel(); const auto crlsz = crlss.size();
    if ( inlsz < 1 || crlsz < 1 )
	return uiStrings::sRemove();
    if ( inlsz < 2 && crlsz < 2 )
	return toUiString( "%1/%2" ).arg( inlss.posStart() )
				    .arg( crlss.posStart() );
    else if ( inlsz < 2 )
	return toUiString( "%1: %2" ).arg( uiStrings::sInline() )
				     .arg( inlss.posStart() );
    else if ( crlsz < 2 )
	return toUiString( "%1: %2" ).arg( uiStrings::sCrossline() )
				     .arg( crlss.posStart() );

    return toUiString( "%2/%3-%4/%5" )
			 .arg( inlss.posStart() ).arg( crlss.posStart() )
			 .arg( inlss.posStop() ).arg( crlss.posStop() );
}


size_type Survey::FullHorSubSel::expectedNrPositions() const
{
    size_type ret = 0;
    if ( chss_ )
	ret = chss_->nrInl() * chss_->nrCrl();
    else
	for ( auto lhss : lhsss_ )
	    ret += lhss->nrTrcs();
    return ret;
}


Survey::HorSubSel& Survey::FullHorSubSel::horSubSel( idx_type idx )
{
    if ( is2D() )
	return lineHorSubSel( idx );
    return cubeHorSubSel();
}


const Survey::HorSubSel& Survey::FullHorSubSel::horSubSel( idx_type idx ) const
{
    return mSelf().horSubSel( idx );
}


LineHorSubSel& Survey::FullHorSubSel::lineHorSubSel( idx_type idx )
{
    return *lhsss_.get( idx );
}


const LineHorSubSel& Survey::FullHorSubSel::lineHorSubSel( idx_type idx ) const
{
    return *lhsss_.get( idx );
}


const LineHorSubSel* Survey::FullHorSubSel::findLineHorSubSel(
					    GeomID geomid ) const
{
    return lhsss_.find( geomid );
}



Survey::SubSelPosIter::SubSelPosIter( const FullHorSubSel& pss )
    : subsel_(pss)
{
}


Survey::SubSelPosIter::SubSelPosIter( const FullSubSel& fss )
    : subsel_(fss.fullHorSubSel())
{
}


Survey::SubSelPosIter::SubSelPosIter( const SubSelPosIter& oth )
    : subsel_(oth.subsel_)
    , lineidx_(oth.lineidx_)
    , trcidx_(oth.trcidx_)
{
}


#define mRetNoNext() { reset(); return false; }

bool Survey::SubSelPosIter::next()
{
    if ( lineidx_ < 0 )
	lineidx_ = 0;
    trcidx_++;

    if ( subsel_.chss_ )
    {
	if ( trcidx_ >= subsel_.chss_->crlSubSel().size() )
	{
	    lineidx_++; trcidx_ = 0;
	    if ( lineidx_ >= subsel_.chss_->inlSubSel().size() )
		mRetNoNext()
	}
    }
    else
    {
	if ( lineidx_ >= subsel_.lhsss_.size() )
	    mRetNoNext()
	const auto& lhss = *subsel_.lhsss_.get( lineidx_ );
	if ( trcidx_ >= lhss.trcNrSubSel().size() )
	{
	    lineidx_++; trcidx_ = 0;
	    if ( lineidx_ >= subsel_.lhsss_.size() )
		mRetNoNext()
	}
    }

    return true;
}


#define mLineIdx() (lineidx_<0 ? 0 : lineidx_)
#define mTrcIdx() (trcidx_<0 ? 0 : trcidx_)


Pos::GeomID Survey::SubSelPosIter::geomID() const
{
    if ( !subsel_.is2D() )
	return GeomID::get3D();

    const auto lidx = mLineIdx();
    return lidx < subsel_.lhsss_.size() ? subsel_.lhsss_.get(lidx)->geomID()
				       : GeomID();
}


pos_type Survey::SubSelPosIter::trcNr() const
{
    const auto tidx = mTrcIdx();
    if ( !subsel_.is2D() )
	return subsel_.chss_->crlSubSel().pos4Idx( tidx );

    const auto lidx = mLineIdx();
    if ( lidx >= subsel_.lhsss_.size() )
	return 0;

    return subsel_.lhsss_.get(lidx)->trcNrSubSel().pos4Idx( tidx );
}


BinID Survey::SubSelPosIter::binID() const
{
    if ( !subsel_.is2D() )
	return subsel_.chss_->binID4RowCol( RowCol(mTrcIdx(),mLineIdx()) );

    const auto lidx = mLineIdx();
    if ( lidx >= subsel_.lhsss_.size() )
	return BinID::udf();

    const auto& lhss = *subsel_.lhsss_.get( lidx );
    const auto trcnr = lhss.trcNrSubSel().pos4Idx( mTrcIdx() );
    const auto coord = lhss.geometry2D().getCoord( trcnr );
    return subsel_.survInfo().transform( coord );
}


Bin2D Survey::SubSelPosIter::bin2D() const
{
    auto geomid = geomID();
    if ( geomid.is3D() )
	{ pErrMsg("2D/3D err"); geomid.setI( 0 ); }
    return Bin2D( geomid, trcNr() );
}
