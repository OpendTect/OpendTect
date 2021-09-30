/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/

#include "cubedata.h"
#include "cubesubsel.h"
#include "linesdata.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "survgeom2d.h"


bool PosInfo::LinesData::isFullyRegular() const
{
    const auto sz = size();
    for ( idx_type lidx=0; lidx<sz; lidx++ )
	if ( get(lidx)->segments_.size() > 1 )
	    return false;
    return true;
}


void PosInfo::LinesData::setTo( const GeomIDSet& gids )
{
    setEmpty();
    for ( const auto gid : gids )
    {
	auto* ld = new PosInfo::LineData( gid.lineNr() );
	Survey::Geometry2D::get( gid ).data().getSegments( *ld );
	add( ld );
    }
}


void PosInfo::LinesData::setToAllLines()
{
    GeomIDSet gids;
    Survey::Geometry2D::getGeomIDs( gids );
    setTo( gids );
}


PosInfo::LinesData::glob_size_type PosInfo::LinesData::totalSizeInside(
					const LineHorSubSelSet& lhsss ) const
{
    glob_size_type nrpos = 0;
    for ( auto lhss : lhsss )
	nrpos += totalSizeInside( *lhss );

    return nrpos;
}


PosInfo::LinesData::glob_size_type PosInfo::LinesData::totalSizeInside(
					const LineHorSubSel& lhss ) const
{
    glob_size_type nrpos = 0;
    for ( auto linedata : *this )
    {
	if ( GeomID(linedata->linenr_) != lhss.geomID() )
	    continue;

	for ( auto seg : linedata->segments_ )
	    for ( pos_type tnr=seg.start; tnr<=seg.stop; tnr+=seg.step )
		if ( lhss.trcNrSubSel().includes(tnr) )
		    nrpos++;
    }

    return nrpos;
}


bool PosInfo::LinesData::hasPosition( const LineHorSubSel& lhss,
				      glob_idx_type gidx ) const
{
    const Bin2D b2d( lhss.atGlobIdx(gidx) );
    return includes( b2d );
}


bool PosInfo::LinesData::hasPosition( const LineHorSubSelSet& lhsss,
				     glob_idx_type gidx ) const
{
    const Bin2D b2d( lhsss.atGlobIdx(gidx) );
    return includes( b2d );
}


PosInfo::CubeData::CubeData( pos_steprg_type inlrg, pos_steprg_type crlrg )
{
    const auto nrinl = inlrg.nrSteps() + 1;
    for ( auto iinl=0; iinl<nrinl; iinl++ )
    {
	auto* ld = new LineData( inlrg.atIndex(iinl) );
	ld->segments_.add( crlrg );
	add( ld );
    }
}


void PosInfo::CubeData::getRanges( pos_rg_type& inlrg,
				   pos_rg_type& crlrg ) const
{
    inlrg.start = inlrg.stop = crlrg.start = crlrg.stop = 0;
    const auto sz = size();
    if ( sz < 1 )
	return;

    bool isfirst = true;
    for ( idx_type iln=0; iln<sz; iln++ )
    {
	const auto& ld = *get( iln );
	if ( ld.segments_.isEmpty() )
	    continue;

	if ( isfirst )
	{
	    isfirst = false;
	    inlrg.start = inlrg.stop = ld.linenr_;
	    crlrg = ld.segments_[0];
	    crlrg.sort();
	}

	inlrg.include( ld.linenr_ );
	for ( auto seg : ld.segments_ )
	    crlrg.include( seg, false );
    }
}


bool PosInfo::CubeData::getInlRange( pos_steprg_type& rg,
				     bool wantsorted ) const
{
    const auto sz = size();
    if ( sz < 1 )
	return false;
    rg.start = rg.stop = get(0)->linenr_;
    if ( sz == 1 )
	{ rg.step = 1; return true; }

    auto prevlnr = rg.stop = get(1)->linenr_;
    rg.step = rg.stop - rg.start;
    bool isreg = rg.step != 0;
    if ( !isreg ) rg.step = 1;

    for ( idx_type lidx=2; lidx<sz; lidx++ )
    {
	const auto newlnr = get(lidx)->linenr_;
	auto newstep = newlnr - prevlnr;
	if ( newstep != rg.step )
	{
	    isreg = false;
	    if ( newstep && abs(newstep) < abs(rg.step) )
	    {
		rg.step = newstep;
		rg.sort( newstep > 0 );
	    }
	}
	rg.include( newlnr, true );
	prevlnr = newlnr;
    }

    rg.sort( wantsorted );
    return isreg;
}


bool PosInfo::CubeData::getCrlRange( pos_steprg_type& rg,
				     bool wantsorted ) const
{
    const auto sz = size();
    if ( sz < 1 )
	return false;

    const auto* ld = first();
    rg = ld->segments_.size() ? ld->segments_[0] : pos_steprg_type(0,0,1);
    bool foundrealstep = rg.start != rg.stop;
    bool isreg = true;

    for ( idx_type lidx=0; lidx<sz; lidx++ )
    {
	ld = get( lidx );
	for ( idx_type icrl=0; icrl<ld->segments_.size(); icrl++ )
	{
	    const auto& seg = ld->segments_[icrl];
	    rg.include( seg.start ); rg.include( seg.stop );

	    if ( seg.step && seg.start != seg.stop )
	    {
		if ( !foundrealstep )
		{
		    rg.step = seg.step;
		    foundrealstep = true;
		}
		else if ( rg.step != seg.step )
		{
		    isreg = false;
		    const pos_type segstep = abs( seg.step );
		    const pos_type rgstep = abs( rg.step );
		    if ( segstep < rgstep )
		    {
			rg.step = seg.step;
			rg.sort( seg.step > 0 );
		    }
		}
	    }
	}
    }

    rg.sort( wantsorted );
    return isreg;
}


bool PosInfo::CubeData::getCubeHorSubSel( CubeHorSubSel& chss ) const
{
    pos_steprg_type inlrg, crlrg;
    bool ret = getInlRange( inlrg );
    ret &= getCrlRange( crlrg );
    chss = CubeHorSubSel( inlrg, crlrg );
    return ret;
}


bool PosInfo::CubeData::isFullyRegular() const
{
    const auto sz = size();
    if ( sz < 1 )
	return true;
    const PosInfo::LineData* ld = first();
    if ( ld->segments_.isEmpty() )
	return sz == 1;

    const auto seg = ld->segments_.first();
    pos_type lnrstep = mUdf(pos_type);
    for ( idx_type lidx=0; lidx<sz; lidx++ )
    {
	ld = get( lidx );
	if ( ld->segments_.size() != 1 || ld->segments_[0] != seg )
	    return false;
	if ( lidx < 1 )
	    continue;

	if ( lidx == 1 )
	    lnrstep = ld->linenr_ - get(lidx-1)->linenr_;
	else if ( ld->linenr_ - get(lidx-1)->linenr_ != lnrstep )
	    return false;
    }

    return true;
}


bool PosInfo::CubeData::isCrlReversed() const
{
    const auto sz = size();
    if ( sz < 1 )
	return false;
    for ( idx_type lidx=0; lidx<sz; lidx++ )
    {
	const auto& ld = *get( lidx );
	if ( ld.segments_.isEmpty() )
	    continue;
	if ( ld.segments_.size() >= 2 )
	{
	    if ( ld.segments_[0].start==ld.segments_[1].start )
		{ pErrMsg( BufferString("Same start: ",ld.linenr_) ); continue;}
	    return ld.segments_[0].start > ld.segments_[1].start;
	}
	else
	{
	    if ( ld.segments_[0].start==ld.segments_[0].stop )
		continue;
	    return ld.segments_[0].start > ld.segments_[0].stop;
	}
    }

    return false;
}


bool PosInfo::CubeData::haveCrlStepInfo() const
{
    const auto sz = size();
    if ( sz < 1 )
	return false;

    for ( idx_type lidx=0; lidx<sz; lidx++ )
    {
	const auto& ld = *get( lidx );
	for ( idx_type icrl=0; icrl<ld.segments_.size(); icrl++ )
	{
	    const auto& seg = ld.segments_[icrl];
	    if ( seg.start != seg.stop )
		return true;
	}
    }

    return false;
}


bool PosInfo::CubeData::isAll( const CubeHorSubSel& hss ) const
{
    pos_steprg_type inlrg;
    if ( !getInlRange(inlrg) || inlrg != hss.inlRange() )
	return false;

    for ( auto ld : *this )
    {
	if ( ld->segments_.size() != 1
	  || ld->segments_[0] != hss.crlRange() )
	    return false;
    }

    return true;
}


void PosInfo::CubeData::generate( BinID start, BinID stop, BinID step,
				  bool allowreversed )
{
    erase();

    if ( !allowreversed )
    {
	if ( start.inl() > stop.inl() )
	    std::swap( start.inl(), stop.inl() );
	if ( start.crl() > stop.crl() )
	    std::swap( start.crl(), stop.crl() );
	if ( step.inl() < 0 )
	    step.inl() = -step.inl();
	if ( step.crl() < 0 )
	    step.crl() = -step.crl();
    }

    const bool isinlrev = step.inl()<0;
    for ( pos_type lnr=start.inl();
	  isinlrev ? lnr>=stop.inl() : lnr<=stop.inl();
	  lnr+=step.inl() )
    {
	LineData* ld = new LineData( lnr );
	ld->segments_.add( pos_steprg_type(start.crl(),stop.crl(),step.crl()) );
	add( ld );
    }
}


void PosInfo::CubeData::fillBySI( OD::SurvLimitType slt )
{
    const CubeHorSubSel hss( SurvGeom::get3D(slt) );
    generate( BinID(hss.inlStart(),hss.crlStart()),
	      BinID(hss.inlStop(),hss.crlStop()),
	      BinID(hss.inlStep(),hss.crlStep()), false );
}


PosInfo::CubeData::glob_size_type PosInfo::CubeData::totalSizeInside(
					const CubeHorSubSel& hss ) const
{
    glob_size_type nrpos = 0;
    for ( auto linedata : *this )
	if ( hss.inlRange().isPresent(linedata->linenr_) )
	    for ( auto seg : linedata->segments_ )
		for ( pos_type crl=seg.start; crl<=seg.stop; crl+=seg.step )
		    if ( hss.crlRange().isPresent(crl) )
			nrpos++;
    return nrpos;
}


bool PosInfo::CubeData::hasPosition( const CubeHorSubSel& hss,
				     glob_idx_type gidx ) const
{
    const BinID bid( hss.atGlobIdx(gidx) );
    return includes( bid );
}


BinID PosInfo::CubeData::minStep() const
{
    const auto sz = size();
    if ( sz < 1 )
	return BinID(1,1);

    BinID minstep( 1, first()->minStep() );
    if ( sz == 1 )
	return minstep;

    minstep.inl() = std::abs( get(1)->linenr_ - first()->linenr_ );
    for ( idx_type iln=1; iln<sz; iln++ )
    {
	const LineData& ld = *get( iln );
	pos_type istp = std::abs( ld.linenr_ - get(iln-1)->linenr_ );
	if ( istp && istp < minstep.inl() )
	    minstep.inl() = istp;
	pos_type cstp = ld.minStep();
	if ( cstp && cstp < minstep.crl() )
	    minstep.crl() = cstp;
    }

    return minstep;
}


BinID PosInfo::CubeData::centerPos() const
{
    if ( isEmpty() )
	return BinID(0,0);

    const LineData& ld = *get( size() / 2 );
    return BinID( ld.linenr_, ld.centerNumber() );
}


BinID PosInfo::CubeData::nearestBinID( const BinID& bid, idx_type mxoff ) const
{
    if ( isEmpty() )
	return BinID(0,0);

    idx_type newidx;
    idx_type inlidx = lineIndexOf( bid.inl(), &newidx );
    if ( inlidx < 0 )
    {
	inlidx = newidx;
	if ( inlidx > size() - 1 )
	    inlidx = size() - 1;
    }
    else if ( get(inlidx)->includes(bid.crl()) )
	return bid; // exact match

    BinID ret( 0, 0 );
    const Coord pos = bid.coord();
    Pos::Ordinate_Type minoff = MAXFLOAT;
    for ( idx_type idx=inlidx-mxoff; idx<=inlidx+mxoff; idx++ )
    {
	if ( !validIdx(idx) )
	    continue;
	const LineData& ld = *get( idx );
	pos_type nearcrl = ld.nearestNumber( bid.crl() );
	const BinID nrbid( ld.linenr_, nearcrl );
	const Pos::Ordinate_Type nroff = pos.sqDistTo( nrbid.coord() );
	if ( nroff < minoff )
	{
	    minoff = nroff;
	    ret = BinID( ld.linenr_, nearcrl );
	}
    }

    return ret;
}


PosInfo::SortedCubeData::idx_type
PosInfo::SortedCubeData::lineIndexOf( pos_type reqlnr, idx_type* newidx ) const
{
    const auto nrld = size();
    if ( nrld < 1 )
	{ if ( newidx ) *newidx = 0; return -1; }

    idx_type loidx = 0;
    pos_type lnr = get(loidx)->linenr_;
    if ( reqlnr <= lnr )
    {
	if ( newidx ) *newidx = 0;
	return reqlnr == lnr ? loidx : -1;
    }
    else if ( nrld == 1 )
	{ if ( newidx ) *newidx = 1; return -1; }

    idx_type hiidx = nrld - 1;
    lnr = get(hiidx)->linenr_;
    if ( reqlnr >= lnr )
    {
	if ( newidx ) *newidx = hiidx+1;
	return reqlnr == lnr ? hiidx : -1;
    }
    else if ( nrld == 2 )
	{ if ( newidx ) *newidx = 1; return -1; }

    while ( hiidx - loidx > 1 )
    {
	const idx_type mididx = (hiidx + loidx) / 2;
	lnr = get(mididx)->linenr_;
	if ( lnr == reqlnr )
	    return mididx;
	else if ( reqlnr > lnr )
	    loidx = mididx;
	else
	    hiidx = mididx;
    }

    if ( newidx )
	*newidx = hiidx;
    return -1;
}


PosInfo::SortedCubeData& PosInfo::SortedCubeData::add( PosInfo::LineData* ld )
{
    return (PosInfo::SortedCubeData&)doAdd( ld );
}


PosInfo::SortedCubeData& PosInfo::SortedCubeData::doAdd( PosInfo::LineData* ld )
{
    if ( !ld ) return *this;

    idx_type newidx;
    const auto curidx = lineIndexOf( ld->linenr_, &newidx );
    if ( curidx < 0 )
    {
	if ( newidx >= size() )
	    ManagedObjectSet<LineData>::doAdd( ld );
	else
	    insertAt( ld, newidx );
	return *this;
    }
    LineData* curld = get( curidx );
    if ( ld == curld )
	return *this;

    curld->merge( *ld, true );
    delete ld;
    return *this;
}
