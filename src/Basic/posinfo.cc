/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/


#include "posinfo.h"
#include "survinfo.h"
#include "survgeom.h"
#include "horsubsel.h"
#include "od_iostream.h"
#include "math2.h"


int PosInfo::LineData::size() const
{
    int res = 0;
    for ( int idx=0; idx<segments_.size(); idx++ )
	res += segments_[idx].nrSteps() + 1;
    return res;
}


bool PosInfo::LineData::operator ==( const PosInfo::LineData& oth ) const
{
    if ( this == &oth )
	return true;

    if ( linenr_ != oth.linenr_ )
	return false;

    const int nrsegs = segments_.size();
    if ( nrsegs != oth.segments_.size() )
	return false;

    for ( int iseg=0; iseg<nrsegs; iseg++ )
    {
	const PosInfo::LineData::Segment& myseg = segments_[iseg];
	const PosInfo::LineData::Segment& othseg = oth.segments_[iseg];
	if ( myseg != othseg )
	    return false;
    }

    return true;
}


int PosInfo::LineData::minStep() const
{
    const int nrsegs = segments_.size();
    if ( nrsegs < 1 )
	return 1;

    int minstep = std::abs( segments_[0].step );
    for ( int iseg=1; iseg<nrsegs; iseg++ )
    {
	const int stp = std::abs( segments_[iseg].step );
	if ( stp && stp < minstep )
	    minstep = stp;
    }

    return minstep;
}


int PosInfo::LineData::centerNumber() const
{
    const int nr = (segments_.first().start + segments_.last().stop) / 2;
    return nearestNumber( nr );
}


int PosInfo::LineData::nearestNumber( int nr ) const
{
    const int nrsegs = segments_.size();
    if ( nrsegs < 1 )
	return -1;

    pos_rg_type nrrg( segments_.first().start, segments_.last().stop );
    const bool isrev = nrrg.start > nrrg.stop;
    nrrg.sort();
    if ( nr <= nrrg.start )
	return nrrg.start;
    if ( nr >= nrrg.stop )
	return nrrg.stop;

    for ( int idx=0; idx<nrsegs; idx++ )
    {
	const int iseg = isrev ? nrsegs-idx-1 : idx;
	const Segment& seg = segments_[iseg];
	if ( isrev )
	{
	    if ( nr > seg.start )
	    {
		if ( idx > 0 )
		{
		    const int prevstop = segments_[iseg-1].stop;
		    if ( prevstop - nr < nr - seg.start )
			return prevstop;
		}
		return seg.start;
	    }
	    if ( nr >= seg.stop )
	    {
		const int segidx = seg.nearestIndex( nr );
		return seg.atIndex( segidx );
	    }
	}
	else
	{
	    if ( nr < seg.start )
	    {
		if ( idx > 0 )
		{
		    const int prevstop = segments_[iseg-1].stop;
		    if ( nr - prevstop < seg.start - nr )
			return prevstop;
		}
		return seg.start;
	    }
	    if ( nr <= seg.stop )
	    {
		const int segidx = seg.nearestIndex( nr );
		return seg.atIndex( segidx );
	    }
	}
    }

    pErrMsg( "Shld not be reachable" );
    return -1;
}


int PosInfo::LineData::nearestSegment( double x ) const
{
    const int nrsegs = segments_.size();
    if ( nrsegs < 1 )
	return -1;

    int ret = 0; float mindist = mUdf(float);
    for ( int iseg=0; iseg<nrsegs; iseg++ )
    {
	const Segment& seg = segments_[iseg];

	const bool isrev = seg.step < 0;
	const float hstep = (float)seg.step * 0.5f;
	float dist;
	if ( (isrev && x > seg.start+hstep) || (!isrev && x < seg.start-hstep) )
	    dist = (float)( x - seg.start );
	else if ( (isrev && x<seg.stop-hstep) || (!isrev && x>seg.stop+hstep))
	    dist = (float)( x - seg.stop );
	else
	    { ret = iseg; break; }

	if ( dist < 0 )
	    dist = -dist;
	if ( dist < mindist )
	    { ret = iseg; mindist = dist; }
    }

    return ret;
}


int PosInfo::LineData::segmentOf( int nr ) const
{
    for ( int iseg=0; iseg<segments_.size(); iseg++ )
    {
	if ( segments_[iseg].includes(nr,false) )
	{
	    if ( segments_[iseg].step < 2 )
		return iseg;

	    const bool inbetween = (nr-segments_[iseg].start)
				   % segments_[iseg].step;
	    return inbetween ? -1 : iseg;
	}
    }

    return -1;
}


PosInfo::LineData::pos_rg_type PosInfo::LineData::range() const
{
    if ( segments_.isEmpty() )
	return pos_rg_type( mUdf(int), mUdf(int) );

    pos_rg_type ret( segments_[0].start, segments_[0].start );
    for ( int idx=0; idx<segments_.size(); idx++ )
    {
	const Segment& seg = segments_[idx];
	if ( seg.start < ret.start ) ret.start = seg.start;
	if ( seg.stop < ret.start ) ret.start = seg.stop;
	if ( seg.start > ret.stop ) ret.stop = seg.start;
	if ( seg.stop > ret.stop ) ret.stop = seg.stop;
    }

    return ret;
}


bool PosInfo::LineData::isValid( const PosInfo::LineDataPos& ldp ) const
{
    if ( ldp.segnr_ < 0 || ldp.segnr_ >= segments_.size() )
	return false;
    return ldp.sidx_ >= 0 && ldp.sidx_ <= segments_[ldp.segnr_].nrSteps();
}


bool PosInfo::LineData::toNext( PosInfo::LineDataPos& ldp ) const
{
    if ( !isValid(ldp) )
    {
	ldp.toStart();
	return isValid(ldp);
    }
    else
    {
	ldp.sidx_++;
	if ( ldp.sidx_ > segments_[ldp.segnr_].nrSteps() )
	{
	    ldp.segnr_++; ldp.sidx_ = 0;
	    if ( ldp.segnr_ >= segments_.size() )
		return false;
	}
	return true;
    }
}


bool PosInfo::LineData::toPrev( PosInfo::LineDataPos& ldp ) const
{
    if ( !isValid(ldp) )
    {
	if ( segments_.isEmpty() )
	    return false;

	ldp.segnr_ = segments_.size() - 1;
	ldp.sidx_ = segments_[ldp.segnr_].nrSteps();
	return true;
    }
    else
    {
	ldp.sidx_--;
	if ( ldp.sidx_ < 0 )
	{
	    ldp.segnr_--;
	    if ( ldp.segnr_ < 0 )
		return false;
	    ldp.sidx_ = segments_[ldp.segnr_].nrSteps();
	}
	return true;
    }
}


void PosInfo::LineData::merge( const PosInfo::LineData& ld1, bool inc )
{
    if ( segments_.isEmpty() )
    {
	if ( inc )
	    segments_ = ld1.segments_;
	return;
    }
    if ( ld1.segments_.isEmpty() )
    {
	if ( !inc )
	    segments_.erase();
	return;
    }

    const PosInfo::LineData ld2( *this );
    segments_.erase();

    pos_rg_type rg( ld1.range() ); rg.include( ld2.range() );
    const int defstep = ld1.segments_.isEmpty() ? ld2.segments_[0].step
						: ld1.segments_[0].step;
    if ( rg.start == rg.stop )
    {
	segments_ += Segment( rg.start, rg.start, defstep );
	return;
    }
    else if ( ld1.segments_.size() == 1 && ld2.segments_.size() == 1 )
    {
	// Very common, can be done real fast
	if ( inc )
	    segments_ += Segment( rg.start, rg.stop, defstep );
	else
	{
	    Segment seg( ld1.segments_[0] );
	    const Segment& ld2seg = ld2.segments_[0];
	    if ( ld2seg.start > seg.start ) seg.start = ld2seg.start;
	    if ( ld2seg.stop < seg.stop ) seg.stop = ld2seg.stop;
	    if ( seg.stop >= seg.start )
		segments_ += seg;
	}
	return;
    }

    // slow but straightforward
    Segment curseg( mUdf(int), 0, mUdf(int) );
    for ( int nr=rg.start; nr<=rg.stop; nr++ )
    {
	const bool in1 = ld1.segmentOf(nr) >= 0;
	bool use = true;
	if ( (!in1 && !inc) || (in1 && inc ) )
	    use = inc;
	else
	    use = ld2.segmentOf(nr) >= 0;

	if ( use )
	{
	    if ( mIsUdf(curseg.start) )
		curseg.start = curseg.stop = nr;
	    else
	    {
		int curstep = nr - curseg.stop;
		if ( mIsUdf(curseg.step) )
		{
		    curseg.step = curstep;
		    curseg.stop = nr;
		}
		else if ( curstep == curseg.step )
		    curseg.stop = nr;
		else
		{
		    segments_ += curseg;
		    curseg.start = curseg.stop = nr;
		    curseg.step = mUdf(int);
		}
	    }
	}
    }

    if ( mIsUdf(curseg.start) )
	return;

    if ( mIsUdf(curseg.step) ) curseg.step = defstep;
    segments_ += curseg;
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
    for ( int iln=start.inl(); isinlrev ? iln>=stop.inl() : iln<=stop.inl();
	      iln+=step.inl() )
    {
	LineData* ld = new LineData( iln );
	ld->segments_ += LineData::Segment( start.crl(), stop.crl(),step.crl());
	*this += ld;
    }
}


void PosInfo::CubeData::fillBySI( OD::SurvLimitType slt )
{
    const CubeHorSubSel hss( SurvGeom::get3D(slt) );
    generate( BinID(hss.inlStart(),hss.crlStart()),
	      BinID(hss.inlStop(),hss.crlStop()),
	      BinID(hss.inlStep(),hss.crlStep()), false );
}


void PosInfo::CubeData::copyContents( const PosInfo::CubeData& cd )
{
    if ( &cd != this )
    {
	erase();
	for ( int idx=0; idx<cd.size(); idx++ )
	    *this += new PosInfo::LineData( *cd[idx] );
    }
}


int PosInfo::CubeData::totalNrSegments() const
{
    int nrseg = 0;
    for ( int idx=0; idx<size(); idx++ )
	nrseg += (*this)[idx]->segments_.size();

    return nrseg;
}


int PosInfo::CubeData::totalSize() const
{
    int nrpos = 0;
    for ( int idx=0; idx<size(); idx++ )
	nrpos += (*this)[idx]->size();

    return nrpos;
}


int PosInfo::CubeData::totalSizeInside( const CubeHorSubSel& hss ) const
{
    int nrpos = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	const PosInfo::LineData* linedata = (*this)[idx];
	if ( !hss.inlRange().isPresent(linedata->linenr_) )
	    continue;

	for ( int idy=0; idy<linedata->segments_.size(); idy++ )
	{
	    const PosInfo::LineData::Segment& segment =
		linedata->segments_[idy];

	    for ( int crl=segment.start; crl<=segment.stop; crl+=segment.step )
	    {
		if ( hss.crlRange().isPresent(crl) )
		    nrpos ++;
	    }
	}
    }

    return nrpos;
}


int PosInfo::CubeData::indexOf( int lnr, int* newidx ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->linenr_ == lnr )
	    return idx;
    return -1;
}


int PosInfo::SortedCubeData::indexOf( int reqlnr, int* newidx ) const
{
    const int nrld = size();
    if ( nrld < 1 )
	{ if ( newidx ) *newidx = 0; return -1; }

    int loidx = 0;
    int lnr = (*this)[loidx]->linenr_;
    if ( reqlnr <= lnr )
    {
	if ( newidx ) *newidx = 0;
	return reqlnr == lnr ? loidx : -1;
    }
    else if ( nrld == 1 )
	{ if ( newidx ) *newidx = 1; return -1; }

    int hiidx = nrld - 1;
    lnr = (*this)[hiidx]->linenr_;
    if ( reqlnr >= lnr )
    {
	if ( newidx ) *newidx = hiidx+1;
	return reqlnr == lnr ? hiidx : -1;
    }
    else if ( nrld == 2 )
	{ if ( newidx ) *newidx = 1; return -1; }

    while ( hiidx - loidx > 1 )
    {
	const int mididx = (hiidx + loidx) / 2;
	lnr = (*this)[mididx]->linenr_;
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


void PosInfo::CubeData::limitTo( const CubeHorSubSel& hss )
{
    for ( int iidx=size()-1; iidx>=0; iidx-- )
    {
	PosInfo::LineData* ld = (*this)[iidx];
	if ( !hss.inlRange().isPresent(ld->linenr_) )
	    { removeSingle( iidx ); continue; }

	int nrvalidsegs = 0;
	for ( int iseg=ld->segments_.size()-1; iseg>=0; iseg-- )
	{
	    auto& seg = ld->segments_[iseg];
	    const bool isrev = seg.start > seg.stop;
	    auto segstart = seg.start;
	    auto segstop = seg.stop;
	    if ( segstart > hss.crlStop() || segstop < hss.crlStart() )
		{ ld->segments_.removeSingle( iseg ); continue; }

	    seg.step = Math::LCMOf( seg.step, hss.crlStep() );
	    if ( !seg.step )
		{ ld->segments_.removeSingle( iseg ); continue; }

	    if ( segstart < hss.crlStart() )
	    {
		int newstart = hss.crlStart();
		int diff = newstart - segstart;
		if ( diff % seg.step )
		{
		    diff += seg.step - diff % seg.step;
		    newstart = segstart + diff;
		}

		if ( isrev )
		    seg.stop = newstart;
		else
		    seg.start = newstart;
	    }
	    if ( segstop > hss.crlStop() )
	    {
		int newstop = hss.crlStop();
		int diff = segstop - newstop;
		if ( diff % seg.step )
		{
		    diff += seg.step - diff % seg.step;
		    newstop = segstop - diff;
		}

		if ( isrev )
		    seg.start = newstop;
		else
		    seg.stop = newstop;
	    }
	    if ( segstart > segstop )
		ld->segments_.removeSingle( iseg );
	    else
		nrvalidsegs++;
	}

	if ( !nrvalidsegs )
	    removeSingle( iidx );
    }
}


bool PosInfo::CubeData::includes( const BinID& bid ) const
{
    return includes( bid.inl(), bid.crl() );
}


bool PosInfo::CubeData::includes( int lnr, int crl ) const
{
    int ilnr = indexOf( lnr ); if ( ilnr < 0 ) return false;
    for ( int iseg=0; iseg<(*this)[ilnr]->segments_.size(); iseg++ )
	if ( (*this)[ilnr]->segments_[iseg].includes(crl,false) )
	    return true;
    return false;
}


void PosInfo::CubeData::getRanges( pos_rg_type& inlrg,
				   pos_rg_type& crlrg ) const
{
    inlrg.start = inlrg.stop = crlrg.start = crlrg.stop = 0;
    const int sz = size();
    if ( sz < 1 )
	return;

    bool isfirst = true;
    for ( int iln=0; iln<sz; iln++ )
    {
	const PosInfo::LineData& ld = *(*this)[iln];
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
	for ( int iseg=0; iseg<ld.segments_.size(); iseg++ )
	    crlrg.include( ld.segments_[iseg], false );
    }
}


bool PosInfo::CubeData::getInlRange( pos_steprg_type& rg,
				     bool wantsorted ) const
{
    const auto sz = size();
    if ( sz < 1 )
	return false;
    rg.start = rg.stop = (*this)[0]->linenr_;
    if ( sz == 1 )
	{ rg.step = 1; return true; }

    int prevlnr = rg.stop = (*this)[1]->linenr_;
    rg.step = rg.stop - rg.start;
    bool isreg = rg.step != 0;
    if ( !isreg ) rg.step = 1;

    for ( int idx=2; idx<sz; idx++ )
    {
	const int newlnr = (*this)[idx]->linenr_;
	int newstep =  newlnr - prevlnr;
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

    const PosInfo::LineData* ld = (*this)[0];
    rg = ld->segments_.size() ? ld->segments_[0] : pos_steprg_type(0,0,1);
    bool foundrealstep = rg.start != rg.stop;
    bool isreg = true;

    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	ld = (*this)[ilnr];
	for ( int icrl=0; icrl<ld->segments_.size(); icrl++ )
	{
	    const PosInfo::LineData::Segment& seg = ld->segments_[icrl];
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
		    const int segstep = abs(seg.step);
		    const int rgstep = abs(rg.step);
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


bool PosInfo::CubeData::isValid( const PosInfo::CubeDataPos& cdp ) const
{
    if ( cdp.lidx_ < 0 || cdp.lidx_ >= size() )
	return false;
    const TypeSet<LineData::Segment>& segs( (*this)[cdp.lidx_]->segments_ );
    if ( cdp.segnr_ < 0 || cdp.segnr_ >= segs.size() )
	return false;
    return cdp.sidx_ >= 0 && cdp.sidx_ <= segs[cdp.segnr_].nrSteps();
}


bool PosInfo::CubeData::isValid( od_int64 gidx, const CubeHorSubSel& hss ) const
{
    const BinID bid( hss.atGlobIdx(gidx) );
    return isValid( bid );
}


bool PosInfo::CubeData::isValid( const BinID& bid ) const
{
    const PosInfo::CubeDataPos cdatapos( cubeDataPos(bid) );
    return isValid( cdatapos );
}


BinID PosInfo::CubeData::minStep() const
{
    const int sz = size();
    if ( sz < 1 )
	return BinID(1,1);

    BinID minstep( 1, first()->minStep() );
    if ( sz == 1 )
	return minstep;

    minstep.inl() = std::abs( (*this)[1]->linenr_ - first()->linenr_ );
    for ( int iln=1; iln<sz; iln++ )
    {
	const LineData& ld = *((*this)[iln]);
	int istp = std::abs( ld.linenr_ - (*this)[iln-1]->linenr_ );
	if ( istp && istp < minstep.inl() )
	    minstep.inl() = istp;
	int cstp = ld.minStep();
	if ( cstp && cstp < minstep.crl() )
	    minstep.crl() = cstp;
    }

    return minstep;
}


BinID PosInfo::CubeData::centerPos() const
{
    if ( isEmpty() )
	return BinID(0,0);

    const LineData& ld = *((*this)[size() / 2]);
    return BinID( ld.linenr_, ld.centerNumber() );
}


BinID PosInfo::CubeData::nearestBinID( const BinID& bid ) const
{
    if ( isEmpty() )
	return BinID(0,0);

    int newidx;
    int inlidx = indexOf( bid.inl(), &newidx );
    if ( inlidx < 0 )
    {
	inlidx = newidx;
	if ( inlidx > size() - 1 )
	    inlidx = size() - 1;
    }
    else if ( (*this)[inlidx]->includes(bid.crl()) )
	return bid; // exact match

    BinID ret( 0, 0 );
    int minnroff = mUdf( int );
    for ( int idx=inlidx-2; idx<=inlidx+2; idx++ )
    {
	if ( !validIdx(idx) )
	    continue;
	const LineData& ld = *((*this)[idx]);
	int nearcrl = ld.nearestNumber( bid.crl() );
	int nroff = std::abs(ld.linenr_-bid.inl())
		  + std::abs(nearcrl-bid.crl());
	if ( nroff < minnroff )
	{
	    minnroff = nroff;
	    ret = BinID( ld.linenr_, nearcrl );
	}
    }

    return ret;
}


bool PosInfo::CubeData::toNext( PosInfo::CubeDataPos& cdp ) const
{
    if ( cdp.lidx_ < 0 || cdp.lidx_ >= size() )
    {
	cdp.toStart();
	return isValid(cdp);
    }
    else if ( cdp.segnr_ < 0 )
	cdp.segnr_ = cdp.sidx_ = 0;
    else
    {
	const auto& segset = get(cdp.lidx_)->segments_;
	cdp.sidx_++;
	if ( cdp.sidx_ > segset.get(cdp.segnr_).nrSteps() )
	{
	    cdp.segnr_++; cdp.sidx_ = 0;
	    if ( cdp.segnr_ >= segset.size() )
	    {
		cdp.lidx_++; cdp.segnr_ = 0;
		if ( cdp.lidx_ >= size() )
		    return false;
	    }
	}
    }
    return true;
}


bool PosInfo::CubeData::toNextLine( PosInfo::CubeDataPos& cdp ) const
{
    cdp.lidx_++;
    if ( cdp.lidx_ >= size() )
	return false;

    cdp.segnr_ = cdp.sidx_ = 0;
    return true;
}


bool PosInfo::CubeData::toPrev( PosInfo::CubeDataPos& cdp ) const
{
    if ( !isValid(cdp) )
    {
	if ( isEmpty() )
	    return false;

	cdp.lidx_ = size() - 1;
	const LineData::SegmentSet& segs = (*this)[cdp.lidx_]->segments_;
	cdp.segnr_ = segs.size() - 1;
	cdp.sidx_ = segs[cdp.segnr_].nrSteps();
	return true;
    }
    else
    {
	cdp.sidx_--;
	if ( cdp.sidx_ < 0 )
	{
	    cdp.segnr_--;
	    if ( cdp.segnr_ < 0 )
	    {
		cdp.lidx_--;
		if ( cdp.lidx_ < 0 )
		    return false;
		cdp.segnr_ = (*this)[cdp.lidx_]->segments_.size() - 1;
	    }
	    cdp.sidx_ = ((*this)[cdp.lidx_]->segments_)[cdp.segnr_].nrSteps();
	}
	return true;
    }
}


BinID PosInfo::CubeData::binID( const PosInfo::CubeDataPos& cdp ) const
{
    return !isValid(cdp) ? BinID(0,0)
	: BinID( (*this)[cdp.lidx_]->linenr_,
		 (*this)[cdp.lidx_]->segments_[cdp.segnr_].atIndex(cdp.sidx_) );
}


PosInfo::CubeDataPos PosInfo::CubeData::cubeDataPos( const BinID& bid ) const
{
    PosInfo::CubeDataPos cdp;
    cdp.lidx_ = indexOf( bid.inl() );
    if ( cdp.lidx_ < 0 )
	return cdp;
    const TypeSet<LineData::Segment>& segs( (*this)[cdp.lidx_]->segments_ );
    for ( int iseg=0; iseg<segs.size(); iseg++ )
    {
	const pos_steprg_type& seg( segs[iseg] );
	if ( seg.includes(bid.crl(),true) )
	{
	    if ( !seg.step || !((bid.crl()-seg.start) % seg.step) )
	    {
		cdp.segnr_ = iseg;
		cdp.sidx_ = seg.getIndex( bid.crl() );
	    }
	    break;
	}
    }
    return cdp;
}


bool PosInfo::CubeData::isFullyRectAndReg() const
{
    const int sz = size();
    if ( sz < 1 ) return true;

    const PosInfo::LineData* ld = (*this)[0];
    if ( ld->segments_.isEmpty() )
	return sz == 1;
    const PosInfo::LineData::Segment seg = ld->segments_[0];

    int lnrstep = mUdf(int);
    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	ld = (*this)[ilnr];
	if ( ld->segments_.isEmpty() )
	    return false;
	if ( ld->segments_.size() > 1 || ld->segments_[0] != seg )
	    return false;
	if ( ilnr > 0 )
	{
	    if ( ilnr == 1 )
		lnrstep = ld->linenr_ - (*this)[ilnr-1]->linenr_;
	    else if ( ld->linenr_ - (*this)[ilnr-1]->linenr_ != lnrstep )
		return false;
	}
    }

    return true;
}


PosInfo::SortedCubeData& PosInfo::SortedCubeData::add( PosInfo::LineData* ld )
{
    return (PosInfo::SortedCubeData&)doAdd( ld );
}


PosInfo::CubeData& PosInfo::SortedCubeData::doAdd( PosInfo::LineData* ld )
{
    if ( !ld ) return *this;

    int newidx;
    const int curidx = indexOf( ld->linenr_, &newidx );
    if ( curidx < 0 )
    {
	if ( newidx >= size() )
	    ManagedObjectSet<LineData>::doAdd( ld );
	else
	    insertAt( ld, newidx );
	return *this;
    }
    LineData* curld = (*this)[curidx];
    if ( ld == curld )
	return *this;

    curld->merge( *ld, true );
    delete ld;
    return *this;
}


bool PosInfo::CubeData::isCrlReversed() const
{
    const int sz = size();
    if ( sz < 1 )
	return false;
    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	const PosInfo::LineData& ld = *(*this)[ilnr];
	if ( ld.segments_.isEmpty() )
	    continue;
	if ( ld.segments_.size() >= 2 )
	{
	    if ( ld.segments_[0].start==ld.segments_[1].start )
	    {
		BufferString msg( "Two segemnts in line nr " );
		msg += ld.linenr_; msg += " have same start";
		pErrMsg( msg );
		continue;
	    }
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
    const int sz = size();
    if ( sz < 1 )
	return false;

    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	const PosInfo::LineData& ld = *(*this)[ilnr];
	for ( int icrl=0; icrl<ld.segments_.size(); icrl++ )
	{
	    const PosInfo::LineData::Segment& seg = ld.segments_[icrl];
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


void PosInfo::CubeData::merge( const PosInfo::CubeData& pd1, bool inc )
{
    PosInfo::CubeData pd2( *this );
    deepErase( *this );

    for ( int iln1=0; iln1<pd1.size(); iln1++ )
    {
	const PosInfo::LineData& ld1 = *pd1[iln1];
	const int iln2 = pd2.indexOf( ld1.linenr_ );
	if ( iln2 < 0 )
	{
	    if ( inc ) *this += new PosInfo::LineData(ld1);
	    continue;
	}

	PosInfo::LineData* ld = new PosInfo::LineData( *pd2[iln2] );
	ld->merge( ld1, inc );
	*this += ld;
    }
    if ( !inc ) return;

    for ( int iln2=0; iln2<pd2.size(); iln2++ )
    {
	const PosInfo::LineData& ld2 = *pd2[iln2];
	const int iln = indexOf( ld2.linenr_ );
	if ( iln < 0 )
	    *this += new PosInfo::LineData(ld2);
    }
}




bool PosInfo::CubeData::read( od_istream& strm, bool asc )
{
    const int intsz = sizeof(int);
    int buf[4]; int itmp = 0;
    if ( asc )
	strm >> itmp;
    else
    {
	strm.getBin( buf, intsz );
	itmp = buf[0];
    }
    const int nrinl = itmp;
    if ( nrinl < 0 )
	return false;

    const pos_rg_type reasonableinls = SI().reasonableRange( true );
    const pos_rg_type reasonablecrls = SI().reasonableRange( false );

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	int linenr = 0, nrseg = 0;
	if ( asc )
	    strm >> linenr >> nrseg;
	else
	{
	    strm.getBin( buf, 2 * intsz );
	    linenr = buf[0];
	    nrseg = buf[1];
	}
	if ( linenr == 0 ) continue;

	if ( !reasonableinls.includes( linenr, false ) )
	    return false;


	PosInfo::LineData* iinf = new PosInfo::LineData( linenr );

	PosInfo::LineData::Segment crls;
	for ( int iseg=0; iseg<nrseg; iseg++ )
	{
	    if ( asc )
		strm >> crls.start >> crls.stop >> crls.step;
	    else
	    {
		strm.getBin( buf, 3 * intsz );
		crls.start = buf[0]; crls.stop = buf[1]; crls.step = buf[2];
	    }

	    if ( !reasonablecrls.includes( crls.start,false ) ||
		 !reasonablecrls.includes( crls.stop, false ) )
		    return false;

	    if ( crls.step<1 )
	    {
		if ( crls.step<0 || crls.start!=crls.stop )
		    return false;
	    }

	    iinf->segments_ += crls;
	}

	*this += iinf;
    }

    return true;
}


bool PosInfo::CubeData::write( od_ostream& strm, bool asc ) const
{
    const int intsz = sizeof( int );
    const int nrinl = this->size();
    if ( asc )
	strm << nrinl << '\n';
    else
	strm.addBin( &nrinl, intsz );

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	const PosInfo::LineData& inlinf = *(*this)[iinl];
	const int nrsegs = inlinf.segments_.size();
	if ( asc )
	    strm << inlinf.linenr_ << ' ' << nrsegs;
	else
	{
	    strm.addBin( &inlinf.linenr_, intsz );
	    strm.addBin( &nrsegs, intsz );
	}

	for ( int iseg=0; iseg<nrsegs; iseg++ )
	{
	    const PosInfo::LineData::Segment& seg = inlinf.segments_[iseg];
	    if ( asc )
		strm << ' ' << seg.start << ' ' << seg.stop << ' ' << seg.step;
	    else
	    {
		strm.addBin( &seg.start, intsz );
		strm.addBin( &seg.stop, intsz );
		strm.addBin( &seg.step, intsz );
	    }
	    if ( asc )
		strm << '\n';
	}

	if ( !strm.isOK() ) return false;
    }

    return true;
}


PosInfo::CubeDataFiller::CubeDataFiller( CubeData& cd )
    : cd_(cd)
    , ld_(0)
{
    initLine();
}


PosInfo::CubeDataFiller::~CubeDataFiller()
{
    finish();
}


PosInfo::LineData* PosInfo::CubeDataFiller::findLine( int lnr )
{
    const int idxof = cd_.indexOf( lnr );
    return idxof < 0 ? 0 : cd_[idxof];
}


void PosInfo::CubeDataFiller::add( const BinID& bid )
{
    if ( !ld_ || ld_->linenr_ != bid.inl() )
    {
	if ( ld_ )
	    finishLine();
	ld_ = findLine( bid.inl() );
	if ( !ld_ )
	    ld_ = new LineData( bid.inl() );
	else
	{
	    if ( ld_->segmentOf(bid.crl()) >= 0 )
		return;
	    mSetUdf(prevcrl); mSetUdf(seg_.step);
	}
    }

    if ( mIsUdf(prevcrl) )
	prevcrl = seg_.start = seg_.stop = bid.crl();
    else
    {
	const int curstep = bid.crl() - prevcrl;
	if ( curstep != 0 )
	{
	    if ( mIsUdf(seg_.step) )
		seg_.step = curstep;
	    else if ( seg_.step != curstep )
	    {
		ld_->segments_ += seg_;
		seg_.start = bid.crl();
		mSetUdf(seg_.step);
	    }
	    prevcrl = seg_.stop = bid.crl();
	}
    }
}


void PosInfo::CubeDataFiller::finish()
{
    if ( ld_ )
	finishLine();
}


void PosInfo::CubeDataFiller::initLine()
{
    ld_ = 0;
    prevcrl = seg_.start = seg_.stop = seg_.step = mUdf(int);
}


void PosInfo::CubeDataFiller::finishLine()
{
    if ( mIsUdf(seg_.start) )
	delete ld_;
    else
    {
	if ( mIsUdf(seg_.step) )
	{
	    if ( ld_->segments_.isEmpty() )
		seg_.step = SI().crlStep();
	    else
		seg_.step = ld_->segments_[0].step;
	}

	ld_->segments_ += seg_;
	cd_ += ld_;
    }

    initLine();
}
