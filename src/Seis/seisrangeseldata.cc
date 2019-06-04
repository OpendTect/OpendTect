/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-1-1998 / Mar 2019
-*/


#include "seisrangeseldata.h"
#include "cubesubsel.h"
#include "iopar.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

mUseType( Seis::SelData, z_type );
mUseType( Seis::SelData, z_rg_type );
mUseType( Seis::SelData, pos_type );
mUseType( Seis::SelData, pos_rg_type );
mUseType( Seis::SelData, idx_type );
mUseType( Seis::SelData, size_type );
typedef StepInterval<z_type> z_steprg_type;
typedef StepInterval<pos_type> pos_steprg_type;


static void shiftZrg( Survey::FullSubSel& ss, const z_rg_type& zrg )
{
    auto outzrg = ss.zSubSel().outputZRange();
    outzrg.start += zrg.start; outzrg.stop += zrg.stop;
    ss.zSubSel().setOutputZRange( outzrg );
}


Seis::RangeSelData::RangeSelData()
    : css_(new CubeSubSel)
    , lsss_(*new LineSubSelSet)
{
}


Seis::RangeSelData::RangeSelData( GeomID gid )
    : lsss_(*new LineSubSelSet)
{
    if ( gid.is2D() )
	lsss_ += new LineSubSel( gid );
    else
	css_ = new CubeSubSel;
}


Seis::RangeSelData::RangeSelData( const BinID& bid )
    : RangeSelData( TrcKey(bid) )
{
}


Seis::RangeSelData::RangeSelData( GeomID gid, trcnr_type tnr )
    : RangeSelData( TrcKey(gid,tnr) )
{
}


Seis::RangeSelData::RangeSelData( const TrcKey& tk )
    : lsss_(*new LineSubSelSet)
{
    if ( tk.is2D() )
	lsss_ += new LineSubSel( tk.geomID(), tk.trcNr() );
    else
	css_ = new CubeSubSel( tk.binID() );
}


Seis::RangeSelData::RangeSelData( const GeomIDSet& gids )
    : lsss_(*new LineSubSelSet)
{
    if ( gids.isEmpty() || !gids.first().is2D() )
	css_ = new CubeSubSel;
    else
	for ( auto gid : gids )
	    lsss_ += new LineSubSel( gid );
}


Seis::RangeSelData::RangeSelData( const CubeSubSel& css )
    : css_(new CubeSubSel(css))
    , lsss_(*new LineSubSelSet)
{
}


Seis::RangeSelData::RangeSelData( const LineSubSel& lss )
    : lsss_(*new LineSubSelSet)
{
    lsss_ += new LineSubSel( lss );
}


Seis::RangeSelData::RangeSelData( const FullSubSel& fss )
    : lsss_(*new LineSubSelSet)
{
    if ( fss.is2D() )
	lsss_ += new LineSubSel( *fss.asLineSubSel() );
    else
	css_ = new CubeSubSel( *fss.asCubeSubSel() );
}


Seis::RangeSelData::RangeSelData( const CubeHorSubSel& chss )
    : css_(new CubeSubSel(chss))
    , lsss_(*new LineSubSelSet)
{
}


Seis::RangeSelData::RangeSelData( const LineHorSubSel& lhss )
    : lsss_(*new LineSubSelSet)
{
    lsss_ += new LineSubSel( lhss );
}


Seis::RangeSelData::RangeSelData( const LineSubSelSet& lsss )
    : lsss_(*new LineSubSelSet)
{
    deepCopy( lsss_, lsss );
}


Seis::RangeSelData::RangeSelData( const LineHorSubSelSet& lhsss )
    : lsss_(*new LineSubSelSet)
{
    for ( auto lhss : lhsss )
	lsss_ += new LineSubSel( *lhss );
}


Seis::RangeSelData::RangeSelData( const TrcKeySampling& tks )
    : lsss_(*new LineSubSelSet)
{
    if ( tks.is2D() )
	lsss_ += new LineSubSel( tks );
    else
	css_ = new CubeSubSel( tks );
}


Seis::RangeSelData::RangeSelData( const TrcKeyZSampling& tkzs )
    : lsss_(*new LineSubSelSet)
{
    if ( tkzs.is2D() )
	lsss_ += new LineSubSel( tkzs );
    else
	css_ = new CubeSubSel( tkzs );
}


Seis::RangeSelData::RangeSelData( const RangeSelData& oth )
    : lsss_(*new LineSubSelSet)
{
    copyFrom( oth );
}


Seis::RangeSelData::RangeSelData( const IOPar& iop )
    : RangeSelData()
{
    usePar( iop );
}


Seis::RangeSelData::~RangeSelData()
{
    clearContents();
    delete &lsss_;
}


Seis::SelDataPosIter* Seis::RangeSelData::posIter() const
{
    return new RangeSelDataPosIter( *this );
}


void Seis::RangeSelData::clearContents()
{
    deleteAndZeroPtr( css_ );
    lsss_.setEmpty();
}


void Seis::RangeSelData::set3D( bool yn )
{
    if ( !yn )
	deleteAndZeroPtr( css_ );
    else
    {
	lsss_.setEmpty();
	if ( !css_ )
	    css_ = new CubeSubSel;
    }
}


void Seis::RangeSelData::doCopyFrom( const SelData& sd )
{
    clearContents();

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const RangeSelData&,oth,sd)
	if ( oth.css_ )
	    css_ = new CubeSubSel( *oth.css_ );
	else
	    lsss_ = oth.lsss_;
	forceall_ = oth.forceall_;
    }
    else if ( !sd.is2D() )
    {
	css_ = new CubeSubSel;
	css_->setInlRange( pos_steprg_type(sd.inlRange(),SI().inlStep()) );
	css_->setCrlRange( pos_steprg_type(sd.crlRange(),SI().crlStep()) );
	css_->setZRange( z_steprg_type(sd.zRange(),SI().zStep()) );
    }
    else
    {
	const auto nrgids = sd.nrGeomIDs();
	for ( auto idx=0; idx<nrgids; idx++ )
	{
	    auto* lss = new LineSubSel( sd.geomID(idx) );
	    lss->setTrcNrRange( pos_steprg_type(sd.trcNrRange(idx)) );
	    lss->setZRange( z_steprg_type(sd.zRange(idx),SI().zStep()) );
	    lsss_.add( lss );
	}
    }
}


void Seis::RangeSelData::merge( const RangeSelData& oth )
{
    if ( oth.forceall_ )
	forceall_ = true;
    else if ( is2D() )
	lsss_.merge( oth.lsss_ );
    else if ( oth.css_ )
	css_->merge( *oth.css_ );
}


bool Seis::RangeSelData::isAll() const
{
    if ( forceall_ )
	return true;
    else if ( css_ )
	return css_->isAll();
    else
	return lsss_.isAll();
}


void Seis::RangeSelData::setToAll()
{
    if ( css_ )
	css_->setToAll();
    else
	lsss_.setToAll();
}


pos_rg_type Seis::RangeSelData::inlRange() const
{
    if ( css_ )
	return forceall_ ? css_->fullInlRange() : css_->inlRange();

    return SelData::inlRange();
}


pos_rg_type Seis::RangeSelData::crlRange() const
{
    if ( css_ )
	return forceall_ ? css_->fullCrlRange() : css_->crlRange();

    return SelData::crlRange();
}


pos_rg_type Seis::RangeSelData::trcNrRange( idx_type iln ) const
{
    if ( css_ )
	return crlRange();
    else if ( iln >= lsss_.size() )
	{ pErrMsg("iln>=sz"); return pos_rg_type(); }

    return lsss_.get(iln)->trcNrRange();
}


z_rg_type Seis::RangeSelData::zRange( idx_type iln ) const
{
    if ( !is2D() )
	return forceall_ ? SelData::zRange() : css_->zRange();

    if ( !lsss_.validIdx(iln) )
	return SelData::zRange();

    if ( forceall_ )
	return Survey::Geometry2D::get(geomID(iln)).zRange();

    return lsss_.get(iln)->zRange();
}


Pos::GeomID Seis::RangeSelData::gtGeomID( idx_type iln ) const
{
    if ( css_ )
	return GeomID::get3D();
    else if ( iln >= lsss_.size() )
	return GeomID();
    return lsss_.get(iln)->geomID();
}


idx_type Seis::RangeSelData::indexOf( GeomID gid ) const
{
    if ( !is2D() )
	return gid.is2D() ? -1 : 0;
    for ( int idx=0; idx<lsss_.size(); idx++ )
	if ( lsss_.get(idx)->geomID() == gid )
	    return idx;
    return -1;
}


void Seis::RangeSelData::setInlRange( const pos_rg_type& rg )
{
    if ( !css_ )
	return;
    if ( rg.hasStep() )
	css_->setInlRange( (const pos_steprg_type&)rg );
    else
	css_->setInlRange( pos_steprg_type(rg,SI().inlStep()) );
}


void Seis::RangeSelData::setCrlRange( const pos_rg_type& rg )
{
    if ( !css_ )
	return;
    if ( rg.hasStep() )
	css_->setCrlRange( (const pos_steprg_type&)rg );
    else
	css_->setCrlRange( pos_steprg_type(rg,SI().crlStep()) );
}


void Seis::RangeSelData::setTrcNrRange( GeomID gid, const pos_rg_type& rg )
{
    const auto idxof = indexOf( gid );
    if ( idxof >= 0 )
	setTrcNrRange( rg, idxof );
}


void Seis::RangeSelData::setTrcNrRange( const pos_rg_type& rg, idx_type idx )
{
    if ( !is2D() )
	setCrlRange( rg );
    else if ( idx >= lsss_.size() )
	{ pErrMsg("idx bad"); }
    else
    {
	auto& lss = *lsss_[idx];
	if ( rg.hasStep() )
	    lss.setTrcNrRange( (const pos_steprg_type&)rg );
	else
	    lss.setTrcNrRange( pos_steprg_type(rg,lss.zRange().step) );
    }
}


void Seis::RangeSelData::setZRange( const z_rg_type& rg, idx_type idx )
{
    Survey::FullSubSel* fss = 0;
    if ( css_ )
	fss = css_;
    else if ( idx < lsss_.size() )
	fss = lsss_[idx];
    if ( !fss )
	return;

    if ( rg.hasStep() )
	fss->setZRange( (const z_steprg_type&)rg );
    else
	fss->setZRange( z_steprg_type(rg,fss->zRange().step) );
}


void Seis::RangeSelData::set( const CubeSubSel& css )
{
    set3D( true );
    *css_ = css;
}


void Seis::RangeSelData::set( const LineSubSel& lss )
{
    set3D( false );
    lsss_.setEmpty();
    lsss_.add( new LineSubSel(lss) );
}


void Seis::RangeSelData::set( const LineSubSelSet& lsss )
{
    set3D( false );
    lsss_ = lsss;
}


void Seis::RangeSelData::setGeomID( GeomID gid )
{
    lsss_.setEmpty();
    addGeomID( gid );
}


void Seis::RangeSelData::addGeomID( GeomID gid )
{
    set3D( !gid.is2D() );
    if ( is2D() )
    {
	const auto idxof = indexOf( gid );
	if ( idxof < 0 )
	    lsss_.add( new LineSubSel( gid ) );
    }
}


void Seis::RangeSelData::doFillPar( IOPar& iop ) const
{
    if ( forceall_ )
	return;

    if ( css_ )
	css_->fillPar( iop );
    else
    {
	for ( int idx=0; idx<lsss_.size(); idx++ )
	{
	    IOPar subiop;
	    lsss_.get(idx)->fillPar( subiop );
	    iop.mergeComp( subiop, toString(idx) );
	}
    }
}


void Seis::RangeSelData::doUsePar( const IOPar& iop )
{
    clearContents();
    set3D( !iop.isPresent(sNrLinesKey()) );

    if ( css_ )
	css_->usePar( iop );
    else
    {
	size_type nrlines = 0;
	iop.get( sNrLinesKey(), nrlines );
	for ( auto idx=0; idx<nrlines; idx++ )
	{
	    PtrMan<IOPar> subpar = iop.subselect( toString(idx) );
	    if ( subpar && !subpar->isEmpty() )
	    {
		bool is2d = true; GeomID gid;
		Survey::SubSel::getInfo( *subpar, is2d, gid );
		if ( is2d && gid.isValid() )
		{
		    auto* lss = new LineSubSel( gid );
		    lss->usePar( iop );
		    lsss_ += lss;
		}
	    }
	}
    }
}


void Seis::RangeSelData::doExtendZ( const z_rg_type& zrg )
{
    if ( css_ )
	shiftZrg( *css_, zrg );
    else
	for ( auto lss : lsss_ )
	    shiftZrg( *lss, zrg );
}


static void addHrgStepout( Pos::IdxSubSelData& ssd, pos_type so )
{
    auto outrg = ssd.outputPosRange();
    outrg.start -= so; outrg.stop += so;
    ssd.setOutputPosRange( outrg );
}


void Seis::RangeSelData::doExtendH( BinID so, BinID sos )
{
    if ( css_ )
    {
	addHrgStepout( css_->inlSubSel(), so.inl() * sos.inl() );
	addHrgStepout( css_->crlSubSel(), so.crl() * sos.crl() );
    }
    else
    {
	for ( auto lss : lsss_ )
	    addHrgStepout( lss->trcNrSubSel(), so.crl() * sos.crl() );
    }
}


int Seis::RangeSelData::selRes3D( const BinID& bid ) const
{
    if ( forceall_ )
	return 0;

    int ret;
    if ( css_ )
    {
	const auto inlrg = css_->inlRange();
	const auto crlrg = css_->crlRange();
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
	const Coord coord = SI().transform( bid );
	ret = 2 + 256 * 2;
	for ( int iln=0; iln<lsss_.size(); iln++ )
	{
	    const auto& lss = *lsss_[iln];
	    const auto rg = lss.trcNrRange();
	    const auto& geom = lss.geometry2D();
	    auto trcnr = geom.nearestTracePosition( coord );
	    if ( trcnr >= 0 )
	    {
		const BinID trcbid( SI().transform(geom.getCoord(trcnr)) );
		if ( trcbid == bid )
		    { ret = 0; break; }
	    }
	}
    }

    return ret;
}


int Seis::RangeSelData::selRes2D( GeomID gid, pos_type trcnr ) const
{
    if ( forceall_ )
	return 0;

    int ret;
    const auto lidx = indexOf( gid );
    if ( lidx < 0 )
	ret = 2 + 256 * 2;
    else
    {
	const auto rg = lsss_[lidx]->trcNrRange();
	int res = rg.start > trcnr || rg.stop < trcnr ? 2 : 0;
	if ( res == 0 )
	    res = (trcnr - rg.start) % rg.step ? 1 : 0;
	ret = 256 * res;
    }
    return ret;
}


uiString Seis::RangeSelData::gtUsrSummary() const
{
    if ( !css_ )
    {
	if ( lsss_.size() < 1 )
	    return toUiString( "-" );

	const auto nrtrcs = expectedNrTraces();
	const auto nrlines = lsss_.size();
	if ( nrlines > 1 )
	    return toUiString("%1: %2 (%3: %4)")
			.arg( uiStrings::sLine(nrlines) ).arg( nrlines )
			.arg( uiStrings::sTrace(nrtrcs) ).arg( nrtrcs );
	const auto& lss = *lsss_.first();
	const auto nrrg = lss.trcNrRange();
	return toUiString( "%1-%2 [%3]" )
		    .arg( nrrg.start ).arg( nrrg.stop )
		    .arg( nameOf(lss.geomID()) );
    }

    const auto& inlss = css_->inlSubSel(); const auto inlsz = inlss.size();
    const auto& crlss = css_->crlSubSel(); const auto crlsz = crlss.size();
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


size_type Seis::RangeSelData::expectedNrTraces() const
{
    size_type ret = 0;
    if ( css_ )
	ret = forceall_ ? (css_->fullInlRange().nrSteps()+1)
		       * (css_->fullCrlRange().nrSteps()+1)
		     : css_->nrInl() * css_->nrCrl();
    else
	for ( auto lss : lsss_ )
	    ret += forceall_ ? lss->fullTrcNrRange().nrSteps()+1
			  : lss->nrTrcs();
    return ret;
}


Survey::FullSubSel& Seis::RangeSelData::subSel( idx_type idx )
{
    if ( css_ )
	return *css_;
    else if ( lsss_.validIdx(idx) )
	return *lsss_.get( idx );
    else
    {
	pErrMsg("Bad idx");
	static LineSubSel dum = LineSubSel( GeomID() );
	return dum;
    }
}


const Survey::FullSubSel& Seis::RangeSelData::subSel( idx_type idx ) const
{
    return mSelf().subSel( idx );
}


LineSubSel& Seis::RangeSelData::lineSubSel( idx_type idx )
{
    return *lsss_.get( idx );
}


const LineSubSel& Seis::RangeSelData::lineSubSel( idx_type idx ) const
{
    return *lsss_.get( idx );
}


bool Seis::RangeSelData::hasFullZRange() const
{
    return css_ ? css_->zSubSel().isAll() : lsss_.hasFullZRange();
}


const LineSubSel* Seis::RangeSelData::findLineSubSel( GeomID geomid ) const
{
    return lsss_.find( geomid );
}


Seis::RangeSelDataPosIter::RangeSelDataPosIter( const RangeSelData& rsd )
    : SelDataPosIter(rsd)
{
}


Seis::RangeSelDataPosIter::RangeSelDataPosIter( const RangeSelDataPosIter& oth )
    : SelDataPosIter(oth)
    , lineidx_(oth.lineidx_)
    , trcidx_(oth.trcidx_)
{
}


#define mRetNoNext() { reset(); return false; }

bool Seis::RangeSelDataPosIter::next()
{
    if ( lineidx_ < 0 )
	lineidx_ = 0;
    trcidx_++;

    const auto& rsd = rangeSelData();
    if ( rsd.css_ )
    {
	if ( trcidx_ >= rsd.css_->crlSubSel().size() )
	{
	    lineidx_++; trcidx_ = 0;
	    if ( lineidx_ >= rsd.css_->inlSubSel().size() )
		mRetNoNext()
	}
    }
    else
    {
	if ( lineidx_ >= rsd.lsss_.size() )
	    mRetNoNext()
	const auto& lss = *rsd.lsss_.get( lineidx_ );
	if ( trcidx_ >= lss.trcNrSubSel().size() )
	{
	    lineidx_++; trcidx_ = 0;
	    if ( lineidx_ >= rsd.lsss_.size() )
		mRetNoNext()
	}
    }

    return true;
}


#define mLineIdx() (lineidx_<0 ? 0 : lineidx_)
#define mTrcIdx() (trcidx_<0 ? 0 : trcidx_)


Pos::GeomID Seis::RangeSelDataPosIter::geomID() const
{
    const auto& rsd = rangeSelData();
    if ( !rsd.is2D() )
	return GeomID::get3D();

    const auto lidx = mLineIdx();
    return lidx < rsd.lsss_.size() ? rsd.lsss_.get(lidx)->geomID() : GeomID();
}


pos_type Seis::RangeSelDataPosIter::trcNr() const
{
    const auto& rsd = rangeSelData();
    const auto tidx = mTrcIdx();
    if ( !rsd.is2D() )
	return rsd.css_->crlSubSel().pos4Idx( tidx );

    const auto lidx = mLineIdx();
    if ( lidx >= rsd.lsss_.size() )
	return 0;

    return rsd.lsss_.get(lidx)->trcNrSubSel().pos4Idx( tidx );
}


BinID Seis::RangeSelDataPosIter::binID() const
{
    const auto& rsd = rangeSelData();
    if ( !rsd.is2D() )
	return rsd.css_->binID4RowCol( RowCol(mTrcIdx(),mLineIdx()) );

    const auto lidx = mLineIdx();
    if ( lidx >= rsd.lsss_.size() )
	return BinID::udf();

    const auto& lss = *rsd.lsss_.get( lidx );
    const auto trcnr = lss.trcNrSubSel().pos4Idx( mTrcIdx() );
    const auto coord = lss.geometry2D().getCoord( trcnr );
    return SI().transform( coord );
}
