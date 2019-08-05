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

mUseType( PosInfo::LineData, idx_type );
mUseType( PosInfo::LineData, size_type );
mUseType( PosInfo::LineData, pos_type );
mUseType( PosInfo::LineData, pos_rg_type );
mUseType( PosInfo::LineData, pos_steprg_type );
mUseType( PosInfo::LineData, Segment );
mUseType( PosInfo::LineCollData, glob_size_type );


size_type PosInfo::LineData::size() const
{
    size_type res = 0;
    for ( auto seg : segments_ )
	res += seg.nrSteps() + 1;
    return res;
}


bool PosInfo::LineData::operator ==( const PosInfo::LineData& oth ) const
{
    if ( this == &oth )
	return true;
    if ( linenr_ != oth.linenr_ )
	return false;
    const size_type nrsegs = segments_.size();
    if ( nrsegs != oth.segments_.size() )
	return false;

    for ( idx_type iseg=0; iseg<nrsegs; iseg++ )
	if ( segments_[iseg] != oth.segments_[iseg] )
	    return false;

    return true;
}


pos_type PosInfo::LineData::minStep() const
{
    const auto nrsegs = segments_.size();
    if ( nrsegs < 1 )
	return 1;

    pos_type minstep = std::abs( segments_[0].step );
    for ( idx_type iseg=1; iseg<nrsegs; iseg++ )
    {
	const auto stp = std::abs( segments_[iseg].step );
	if ( stp && stp < minstep )
	    minstep = stp;
    }

    return minstep;
}


pos_type PosInfo::LineData::centerNumber() const
{
    if ( segments_.isEmpty() )
	return 0;

    const pos_type nr = (segments_.first().start + segments_.last().stop) / 2;
    return nearestNumber( nr );
}


pos_type PosInfo::LineData::nearestNumber( pos_type nr ) const
{
    const auto nrsegs = segments_.size();
    if ( nrsegs < 1 )
	return -1;

    pos_rg_type nrrg( segments_.first().start, segments_.last().stop );
    const bool isrev = nrrg.start > nrrg.stop;
    nrrg.sort();
    if ( nr <= nrrg.start )
	return nrrg.start;
    if ( nr >= nrrg.stop )
	return nrrg.stop;

    for ( idx_type idx=0; idx<nrsegs; idx++ )
    {
	const idx_type iseg = isrev ? nrsegs-idx-1 : idx;
	const Segment& seg = segments_[iseg];
	if ( isrev )
	{
	    if ( nr > seg.start )
	    {
		if ( idx > 0 )
		{
		    const auto prevstop = segments_[iseg-1].stop;
		    if ( prevstop - nr < nr - seg.start )
			return prevstop;
		}
		return seg.start;
	    }
	    if ( nr >= seg.stop )
	    {
		const auto segidx = seg.nearestIndex( nr );
		return seg.atIndex( segidx );
	    }
	}
	else
	{
	    if ( nr < seg.start )
	    {
		if ( idx > 0 )
		{
		    const auto prevstop = segments_[iseg-1].stop;
		    if ( nr - prevstop < seg.start - nr )
			return prevstop;
		}
		return seg.start;
	    }
	    if ( nr <= seg.stop )
	    {
		const auto segidx = seg.nearestIndex( nr );
		return seg.atIndex( segidx );
	    }
	}
    }

    pErrMsg( "Shld not be reachable" );
    return -1;
}


idx_type PosInfo::LineData::nearestSegment( double x ) const
{
    const auto nrsegs = segments_.size();
    if ( nrsegs < 1 )
	return -1;

    idx_type ret = 0; float mindist = mUdf(float);
    for ( idx_type iseg=0; iseg<nrsegs; iseg++ )
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


idx_type PosInfo::LineData::segmentOf( pos_type nr ) const
{
    for ( idx_type iseg=0; iseg<segments_.size(); iseg++ )
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
	return pos_rg_type( mUdf(pos_type), mUdf(pos_type) );

    pos_rg_type ret( segments_[0].start, segments_[0].start );
    for ( idx_type idx=0; idx<segments_.size(); idx++ )
    {
	const Segment& seg = segments_[idx];
	if ( seg.start < ret.start ) ret.start = seg.start;
	if ( seg.stop < ret.start ) ret.start = seg.stop;
	if ( seg.start > ret.stop ) ret.stop = seg.start;
	if ( seg.stop > ret.stop ) ret.stop = seg.stop;
    }

    return ret;
}


bool PosInfo::LineData::isValid( const LinePos& lp ) const
{
    if ( lp.segnr_ < 0 || lp.segnr_ >= segments_.size() )
	return false;
    return lp.sidx_ >= 0 && lp.sidx_ <= segments_[lp.segnr_].nrSteps();
}


bool PosInfo::LineData::toNext( LinePos& lp ) const
{
    if ( !isValid(lp) )
    {
	lp.toStart();
	return isValid(lp);
    }
    else
    {
	lp.sidx_++;
	if ( lp.sidx_ > segments_[lp.segnr_].nrSteps() )
	{
	    lp.segnr_++; lp.sidx_ = 0;
	    if ( lp.segnr_ >= segments_.size() )
		return false;
	}
	return true;
    }
}


bool PosInfo::LineData::toPrev( LinePos& lp ) const
{
    if ( !isValid(lp) )
    {
	if ( segments_.isEmpty() )
	    return false;

	lp.segnr_ = segments_.size() - 1;
	lp.sidx_ = segments_[lp.segnr_].nrSteps();
	return true;
    }
    else
    {
	lp.sidx_--;
	if ( lp.sidx_ < 0 )
	{
	    lp.segnr_--;
	    if ( lp.segnr_ < 0 )
		return false;
	    lp.sidx_ = segments_[lp.segnr_].nrSteps();
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
    const auto defstep = ld1.segments_.isEmpty() ? ld2.segments_[0].step
						 : ld1.segments_[0].step;
    if ( rg.start == rg.stop )
    {
	segments_.add( Segment(rg.start,rg.start,defstep) );
	return;
    }
    else if ( ld1.segments_.size() == 1 && ld2.segments_.size() == 1 )
    {
	// Very common, can be done real fast
	if ( inc )
	    segments_.add( Segment(rg.start,rg.stop,defstep) );
	else
	{
	    Segment seg( ld1.segments_[0] );
	    const Segment& ld2seg = ld2.segments_[0];
	    if ( ld2seg.start > seg.start ) seg.start = ld2seg.start;
	    if ( ld2seg.stop < seg.stop ) seg.stop = ld2seg.stop;
	    if ( seg.stop >= seg.start )
		segments_.add( seg );
	}
	return;
    }

    // slow but straightforward
    Segment curseg( mUdf(pos_type), 0, mUdf(pos_type) );
    for ( pos_type nr=rg.start; nr<=rg.stop; nr++ )
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
		pos_type curstep = nr - curseg.stop;
		if ( mIsUdf(curseg.step) )
		{
		    curseg.step = curstep;
		    curseg.stop = nr;
		}
		else if ( curstep == curseg.step )
		    curseg.stop = nr;
		else
		{
		    segments_.add( curseg );
		    curseg.start = curseg.stop = nr;
		    curseg.step = mUdf(pos_type);
		}
	    }
	}
    }

    if ( mIsUdf(curseg.start) )
	return;

    if ( mIsUdf(curseg.step) )
	curseg.step = defstep;
    segments_.add( curseg );
}


bool PosInfo::LineCollData::operator ==( const LineCollData& oth ) const
{
    const auto sz = size();
    if ( sz != oth.size() )
	return false;
    if ( sz < 1 )
	return true;

    for ( idx_type idx=0; idx<sz; idx++ )
	if ( *get(idx) != *oth.get(idx) )
	    return false;

    return true;
}


void PosInfo::LineCollData::copyContents( const LineCollData& oth )
{
    if ( &oth != this )
    {
	erase();
	for ( idx_type idx=0; idx<oth.size(); idx++ )
	    add( new LineData( *oth.get(idx) ) );
    }
}


glob_size_type PosInfo::LineCollData::totalSize() const
{
    glob_size_type nrpos = 0;
    for ( auto ld : *this )
	nrpos += ld->size();
    return nrpos;
}


glob_size_type PosInfo::LineCollData::totalNrSegments() const
{
    glob_size_type nrseg = 0;
    for ( auto ld : *this )
	nrseg += ld->segments_.size();
    return nrseg;
}


void PosInfo::LineCollData::merge( const LineCollData& lcd1, bool inc )
{
    const LineCollData lcd2( *this );
    deepErase( *this );

    for ( idx_type iln1=0; iln1<lcd1.size(); iln1++ )
    {
	const LineData& ld1 = *lcd1[iln1];
	const idx_type iln2 = lcd2.lineIndexOf( ld1.linenr_ );
	if ( iln2 < 0 )
	{
	    if ( inc )
		add( new LineData(ld1) );
	    continue;
	}

	LineData* ld = new LineData( *lcd2[iln2] );
	ld->merge( ld1, inc );
	add( ld );
    }
    if ( !inc )
	return;

    for ( idx_type iln2=0; iln2<lcd2.size(); iln2++ )
    {
	const LineData& ld2 = *lcd2[iln2];
	const idx_type iln = lineIndexOf( ld2.linenr_ );
	if ( iln < 0 )
	    add( new LineData(ld2) );
    }
}


bool PosInfo::LineCollData::read( od_istream& strm, bool asc )
{
    const auto intsz = sizeof(int);
    int buf[4]; int itmp = 0;
    if ( asc )
	strm >> itmp;
    else
    {
	strm.getBin( buf, intsz );
	itmp = buf[0];
    }
    const size_type nrinl = itmp;
    if ( nrinl < 0 )
	return false;

    const pos_rg_type reasonableinls = SI().reasonableRange( true );
    const pos_rg_type reasonablecrls = SI().reasonableRange( false );

    for ( idx_type iinl=0; iinl<nrinl; iinl++ )
    {
	pos_type linenr = 0;
        size_type nrseg = 0;
	if ( asc )
	    strm >> linenr >> nrseg;
	else
	{
	    strm.getBin( buf, 2 * intsz );
	    linenr = (pos_type)buf[0];
	    nrseg = (size_type)buf[1];
	}
	if ( linenr == 0 ) continue;

	if ( !reasonableinls.includes( linenr, false ) )
	    return false;


	LineData* iinf = new LineData( linenr );

	Segment crls;
	for ( idx_type iseg=0; iseg<nrseg; iseg++ )
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

	    iinf->segments_.add( crls );
	}

	add( iinf );
    }

    return true;
}


bool PosInfo::LineCollData::write( od_ostream& strm, bool asc ) const
{
    const auto intsz = sizeof( int );
    const auto nrinl = this->size();
    if ( asc )
	strm << nrinl << '\n';
    else
	strm.addBin( &nrinl, intsz );

    for ( idx_type iinl=0; iinl<nrinl; iinl++ )
    {
	const LineData& inlinf = *get( iinl );
	const idx_type nrsegs = inlinf.segments_.size();
	if ( asc )
	    strm << inlinf.linenr_ << ' ' << nrsegs;
	else
	{
	    strm.addBin( &inlinf.linenr_, intsz );
	    strm.addBin( &nrsegs, intsz );
	}

	for ( idx_type iseg=0; iseg<nrsegs; iseg++ )
	{
	    const Segment& seg = inlinf.segments_[iseg];
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


idx_type PosInfo::LineCollData::lineIndexOf( pos_type lnr,
					   idx_type* newidx ) const
{
    for ( idx_type idx=0; idx<size(); idx++ )
	if ( get(idx)->linenr_ == lnr )
	    return idx;

    if ( newidx )
	*newidx = size();
    return -1;
}


bool PosInfo::LineCollData::includes( const BinID& bid ) const
{
    return includes( bid.inl(), bid.crl() );
}


bool PosInfo::LineCollData::includes( const Bin2D& b2d ) const
{
    return includes( b2d.lineNr(), b2d.trcNr() );
}


bool PosInfo::LineCollData::includes( pos_type lnr, pos_type tnr ) const
{
    idx_type lidx = lineIndexOf( lnr );
    if ( lidx < 0 )
	return false;

    const auto& segs = get( lidx )->segments_;
    for ( idx_type iseg=0; iseg<segs.size(); iseg++ )
	if ( segs[iseg].includes(tnr,false) )
	    return true;

    return false;
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
	    const Segment& seg = ld->segments_[icrl];
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


bool PosInfo::LineCollData::isValid( const PosInfo::LineCollPos& lcp ) const
{
    if ( lcp.lidx_ < 0 || lcp.lidx_ >= size() )
	return false;
    const auto& segs( get(lcp.lidx_)->segments_ );
    if ( lcp.segnr_ < 0 || lcp.segnr_ >= segs.size() )
	return false;
    return lcp.sidx_ >= 0 && lcp.sidx_ <= segs[lcp.segnr_].nrSteps();
}


bool PosInfo::LineCollData::toNext( LineCollPos& lcp ) const
{
    if ( lcp.lidx_ < 0 || lcp.lidx_ >= size() )
    {
	lcp.toStart();
	return isValid(lcp);
    }
    else if ( lcp.segnr_ < 0 )
	lcp.segnr_ = lcp.sidx_ = 0;
    else
    {
	const auto& segset = get(lcp.lidx_)->segments_;
	lcp.sidx_++;
	if ( lcp.sidx_ > segset.get(lcp.segnr_).nrSteps() )
	{
	    lcp.segnr_++; lcp.sidx_ = 0;
	    if ( lcp.segnr_ >= segset.size() )
	    {
		lcp.lidx_++; lcp.segnr_ = 0;
		if ( lcp.lidx_ >= size() )
		    return false;
	    }
	}
    }
    return true;
}


bool PosInfo::LineCollData::toNextLine( LineCollPos& lcp ) const
{
    lcp.lidx_++;
    if ( lcp.lidx_ >= size() )
	return false;

    lcp.segnr_ = lcp.sidx_ = 0;
    return true;
}


bool PosInfo::LineCollData::toPrev( LineCollPos& lcp ) const
{
    if ( !isValid(lcp) )
    {
	if ( isEmpty() )
	    return false;

	lcp.lidx_ = size() - 1;
	const auto& segs = get(lcp.lidx_)->segments_;
	lcp.segnr_ = segs.size() - 1;
	lcp.sidx_ = segs[lcp.segnr_].nrSteps();
	return true;
    }
    else
    {
	lcp.sidx_--;
	if ( lcp.sidx_ < 0 )
	{
	    lcp.segnr_--;
	    if ( lcp.segnr_ < 0 )
	    {
		lcp.lidx_--;
		if ( lcp.lidx_ < 0 )
		    return false;
		lcp.segnr_ = get(lcp.lidx_)->segments_.size() - 1;
	    }
	    lcp.sidx_ = get(lcp.lidx_)->segments_.get(lcp.segnr_).nrSteps();
	}
	return true;
    }
}


BinID PosInfo::LineCollData::binID( const LineCollPos& lcp ) const
{
    return !isValid(lcp) ? BinID(0,0)
	: BinID( get(lcp.lidx_)->linenr_,
		 get(lcp.lidx_)->segments_.get(lcp.segnr_).atIndex(lcp.sidx_) );
}


Bin2D PosInfo::LineCollData::bin2D( const LineCollPos& lcp ) const
{
    return Bin2D::decode( binID(lcp) );
}


PosInfo::LineCollPos PosInfo::LineCollData::lineCollPos(
						const BinID& bid ) const
{
    LineCollPos lcp;
    lcp.lidx_ = lineIndexOf( bid.inl() );
    if ( lcp.lidx_ < 0 )
	return lcp;
    const auto& segs( get(lcp.lidx_)->segments_ );
    for ( idx_type iseg=0; iseg<segs.size(); iseg++ )
    {
	const auto& seg( segs[iseg] );
	if ( seg.includes(bid.crl(),true) )
	{
	    if ( !seg.step || !((bid.crl()-seg.start) % seg.step) )
	    {
		lcp.segnr_ = iseg;
		lcp.sidx_ = seg.getIndex( bid.crl() );
	    }
	    break;
	}
    }
    return lcp;
}


PosInfo::LineCollPos PosInfo::LineCollData::lineCollPos(
						const Bin2D& b2d ) const
{
    return lineCollPos( BinID(b2d.idxPair()) );
}


bool PosInfo::CubeData::isFullyRectAndReg() const
{
    const auto sz = size();
    if ( sz < 1 ) return true;

    const PosInfo::LineData* ld = first();
    if ( ld->segments_.isEmpty() )
	return sz == 1;
    const Segment seg = ld->segments_[0];

    pos_type lnrstep = mUdf(pos_type);
    for ( idx_type lidx=0; lidx<sz; lidx++ )
    {
	ld = get( lidx );
	if ( ld->segments_.isEmpty() )
	    return false;
	if ( ld->segments_.size() > 1 || ld->segments_[0] != seg )
	    return false;
	if ( lidx > 0 )
	{
	    if ( lidx == 1 )
		lnrstep = ld->linenr_ - get(lidx-1)->linenr_;
	    else if ( ld->linenr_ - get(lidx-1)->linenr_ != lnrstep )
		return false;
	}
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
	ld->segments_.add( Segment(start.crl(),stop.crl(),step.crl()) );
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


glob_size_type PosInfo::CubeData::totalSizeInside(
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


BinID PosInfo::CubeData::nearestBinID( const BinID& bid ) const
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
    pos_type minnroff = mUdf( pos_type );
    for ( idx_type idx=inlidx-2; idx<=inlidx+2; idx++ )
    {
	if ( !validIdx(idx) )
	    continue;
	const LineData& ld = *get( idx );
	pos_type nearcrl = ld.nearestNumber( bid.crl() );
	pos_type nroff = std::abs( ld.linenr_-bid.inl() )
			  + std::abs( nearcrl-bid.crl() );
	if ( nroff < minnroff )
	{
	    minnroff = nroff;
	    ret = BinID( ld.linenr_, nearcrl );
	}
    }

    return ret;
}


void PosInfo::CubeData::limitTo( const CubeHorSubSel& hss )
{
    for ( idx_type lidx=size()-1; lidx>=0; lidx-- )
    {
	PosInfo::LineData* ld = get( lidx );
	if ( !hss.inlRange().isPresent(ld->linenr_) )
	    { removeSingle( lidx ); continue; }

	size_type nrvalidsegs = 0;
	for ( idx_type iseg=ld->segments_.size()-1; iseg>=0; iseg-- )
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
		auto newstart = hss.crlStart();
		auto diff = newstart - segstart;
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
		auto newstop = hss.crlStop();
		auto diff = segstop - newstop;
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
	    removeSingle( lidx );
    }
}


idx_type PosInfo::SortedCubeData::lineIndexOf( pos_type reqlnr,
					    idx_type* newidx ) const
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


PosInfo::CubeData& PosInfo::SortedCubeData::doAdd( PosInfo::LineData* ld )
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


PosInfo::LineDataFiller::LineDataFiller( LineData& ld )
    : ld_(ld)
{
    reset();
}


PosInfo::LineDataFiller& PosInfo::LineDataFiller::add( pos_type nr )
{
    if ( mIsUdf(nr) )
	return *this;
    else if ( mIsUdf(prevnr_) )
	{ prevnr_ = seg_.start = seg_.stop = nr; return *this; }

    const auto curstep = nr - prevnr_;
    if ( curstep == 0 )
	return *this;

    if ( mIsUdf(seg_.step) )
	seg_.step = curstep;
    else if ( seg_.step != curstep )
    {
	ld_.segments_.add( seg_ );
	seg_.start = nr;
	mSetUdf(seg_.step);
    }

    prevnr_ = seg_.stop = nr;
    return *this;
}


void PosInfo::LineDataFiller::reset()
{
    ld_.segments_.setEmpty();
    prevnr_ = seg_.start = seg_.stop = seg_.step = mUdf( pos_type );
    finished_ = false;
}


bool PosInfo::LineDataFiller::finish()
{
    finished_ = true;
    if ( mIsUdf(seg_.start) )
	return false;

    if ( mIsUdf(seg_.step) && !ld_.segments_.isEmpty() )
	seg_.step = ld_.segments_.last().step;

    ld_.segments_.add( seg_ );
    return true;
}


PosInfo::LineCollDataFiller::LineCollDataFiller( LineCollData& lcd )
    : lcd_(lcd)
{
    reset();
}


void PosInfo::LineCollDataFiller::reset()
{
    deleteAndZeroPtr( ldf_ );
    deleteAndZeroPtr( ld_ );
    lcd_.setEmpty();
}


PosInfo::LineCollDataFiller& PosInfo::LineCollDataFiller::doAdd(
					const IdxPair& ip )
{
    if ( !ld_ || ld_->linenr_ != ip.lineNr() )
    {
	finishLine();
	ld_ = new LineData( ip.lineNr() );
	ldf_ = new LineDataFiller( *ld_ );
    }

    ldf_->add( ip.trcNr() );
    return *this;
}


void PosInfo::LineCollDataFiller::finish()
{
    finishLine();
}


void PosInfo::LineCollDataFiller::finishLine()
{
    if ( !ld_ )
	return;

    if ( ldf_->finish() )
	lcd_.add( ld_ );
    else
	delete ld_;

    deleteAndZeroPtr( ldf_ );
    ld_ = nullptr;
}
