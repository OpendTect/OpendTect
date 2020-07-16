/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/


#include "cubedata.h"
#include "cubesubsel.h"
#include "fullsubsel.h"
#include "linesdata.h"
#include "linesubsel.h"
#include "math2.h"
#include "od_iostream.h"
#include "trckey.h"

mUseType( Pos, GeomID );
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
	const auto& seg = segments_[iseg];
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


bool PosInfo::LineData::isValid( const LineDataPos& ldp ) const
{
    if ( ldp.segnr_ < 0 || ldp.segnr_ >= segments_.size() )
	return false;
    return ldp.sidx_ >= 0 && ldp.sidx_ <= segments_[ldp.segnr_].nrSteps();
}


bool PosInfo::LineData::toNext( LineDataPos& ldp ) const
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


bool PosInfo::LineData::toPrev( LineDataPos& ldp ) const
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


void PosInfo::LineData::merge( const PosInfo::LineData& oth, bool inc )
{
    if ( segments_.isEmpty() )
    {
	if ( inc )
	    segments_ = oth.segments_;
	return;
    }
    if ( oth.segments_.isEmpty() )
    {
	if ( !inc )
	    segments_.erase();
	return;
    }

    const PosInfo::LineData cln( *this );
    segments_.erase();

    pos_rg_type rg( oth.range() ); rg.include( cln.range() );
    const auto defstep = oth.segments_.isEmpty() ? cln.segments_[0].step
						 : oth.segments_[0].step;
    if ( rg.start == rg.stop )
    {
	segments_.add( Segment(rg.start,rg.start,defstep) );
	return;
    }
    else if ( oth.segments_.size() == 1 && cln.segments_.size() == 1 )
    {
	// Very common, can be done real fast
	if ( inc )
	    segments_.add( Segment(rg.start,rg.stop,defstep) );
	else
	{
	    Segment seg( oth.segments_[0] );
	    const Segment& clnseg = cln.segments_[0];
	    if ( clnseg.start > seg.start ) seg.start = clnseg.start;
	    if ( clnseg.stop < seg.stop ) seg.stop = clnseg.stop;
	    if ( seg.stop >= seg.start )
		segments_.add( seg );
	}
	return;
    }

    // slow but straightforward
    Segment curseg( mUdf(pos_type), 0, mUdf(pos_type) );
    for ( pos_type nr=rg.start; nr<=rg.stop; nr++ )
    {
	const bool in1 = oth.segmentOf(nr) >= 0;
	bool use = true;
	if ( (!in1 && !inc) || (in1 && inc ) )
	    use = inc;
	else
	    use = cln.segmentOf(nr) >= 0;

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


PosInfo::CubeData* PosInfo::LineCollData::asCubeData()
{
    return isCubeData() ? static_cast<CubeData*>(this) : nullptr;
}


const PosInfo::CubeData* PosInfo::LineCollData::asCubeData() const
{
    return isCubeData() ? static_cast<const CubeData*>(this) : nullptr;
}


PosInfo::LinesData* PosInfo::LineCollData::asLinesData()
{
    return isLinesData() ? static_cast<LinesData*>(this) : nullptr;
}


const PosInfo::LinesData* PosInfo::LineCollData::asLinesData() const
{
    return isLinesData() ? static_cast<const LinesData*>(this) : nullptr;
}


bool PosInfo::LineCollData::isLineSorted() const
{
    if ( isEmpty() )
	return true;
    auto prevlnr = first()->linenr_;
    for ( auto iln=1; iln<size(); iln++ )
    {
	const auto lnr = get(iln)->linenr_;
	if ( lnr < prevlnr )
	    return false;
	prevlnr = lnr;
    }
    return true;
}


PosInfo::LineCollData* PosInfo::LineCollData::create( const FullHorSubSel& fhss)
{
    LineCollData* ret = nullptr;
    if ( !fhss.is2D() )
    {
	const auto& chss = fhss.cubeHorSubSel();
	ret = new CubeData( chss.inlRange(), chss.crlRange() );
    }
    else
    {
	ret = new LinesData;
	const auto nrlines = fhss.nrGeomIDs();
	for ( auto iln=0; iln<nrlines; iln++ )
	{
	    const auto& lss = fhss.lineHorSubSel( iln );
	    auto* ld = new LineData( fhss.geomID(iln).lineNr() );
	    ld->segments_.add( lss.trcNrRange() );
	    ret->add( ld );
	}
    }

    return ret;
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


glob_size_type PosInfo::LineCollData::totalSizeInside(
					const Survey::HorSubSel& hss ) const
{
    if ( isCubeData() )
    {
	const CubeData* cd = asCubeData();
	const CubeHorSubSel* chss = hss.asCubeHorSubSel();
	if ( chss )
	    return cd->totalSizeInside( *chss );
    }
    else
    {
	const LinesData* ld = asLinesData();
	const LineHorSubSel* lhss = hss.asLineHorSubSel();
	if ( lhss )
	    return ld->totalSizeInside( *lhss );
    }

    return 0;
}


glob_size_type PosInfo::LineCollData::totalNrSegments() const
{
    glob_size_type nrseg = 0;
    for ( auto ld : *this )
	nrseg += ld->segments_.size();
    return nrseg;
}


void PosInfo::LineCollData::limitTo( const Survey::HorSubSel& hss )
{
    const auto* lhss = hss.asLineHorSubSel();
    const auto* chss = hss.asCubeHorSubSel();
    for ( idx_type lidx=size()-1; lidx>=0; lidx-- )
    {
	auto& ld = *get( lidx );
	if ( (lhss && lhss->geomID().lineNr() != ld.linenr_)
	  || (chss && !chss->inlRange().isPresent(ld.linenr_)) )
	    { removeSingle( lidx ); continue; }

	size_type nrvalidsegs = 0;
	for ( idx_type iseg=ld.segments_.size()-1; iseg>=0; iseg-- )
	{
	    auto& seg = ld.segments_[iseg];
	    const bool isrev = seg.start > seg.stop;
	    auto segstart = seg.start;
	    auto segstop = seg.stop;
	    if ( segstart > hss.trcNrStop() || segstop < hss.trcNrStart() )
		{ ld.segments_.removeSingle( iseg ); continue; }

	    seg.step = Math::LCMOf( seg.step, hss.trcNrStep() );
	    if ( !seg.step )
		{ ld.segments_.removeSingle( iseg ); continue; }

	    if ( segstart < hss.trcNrStart() )
	    {
		auto newstart = hss.trcNrStart();
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
	    if ( segstop > hss.trcNrStop() )
	    {
		auto newstop = hss.trcNrStop();
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
		ld.segments_.removeSingle( iseg );
	    else
		nrvalidsegs++;
	}

	if ( !nrvalidsegs )
	    removeSingle( lidx );
    }
}


void PosInfo::LineCollData::merge( const LineCollData& oth, bool inc )
{
    const auto* myclone = clone();
    deepErase( *this );

    for ( idx_type iothln=0; iothln<oth.size(); iothln++ )
    {
	const LineData& ldoth = *oth.get( iothln );
	const idx_type iclln = myclone->lineIndexOf( ldoth.linenr_ );
	if ( iclln < 0 )
	{
	    if ( inc )
		add( new LineData(ldoth) );
	    continue;
	}

	LineData* newld = new LineData( *myclone->get(iclln) );
	newld->merge( ldoth, inc );
	add( newld );
    }

    if ( inc )
    {
	for ( idx_type iclln=0; iclln<myclone->size(); iclln++ )
	{
	    const LineData& ld = *myclone->get( iclln );
	    const idx_type iln = lineIndexOf( ld.linenr_ );
	    if ( iln < 0 )
		add( new LineData(ld) );
	}
    }

    delete myclone;
}


void PosInfo::LineCollData::getFullHorSubSel( FullHorSubSel& fhss,
					      bool is2d ) const
{
    if ( is2d )
    {
	fhss.setEmpty();
	for ( int iln=0; iln<size(); iln++ )
	{
	    const auto& ld = *get( iln );
	    fhss.addGeomID( GeomID(ld.linenr_) );
	    const auto ldrg = ld.range();
	    fhss.lineHorSubSel(iln).trcNrSubSel().setInputPosRange(
			pos_steprg_type(ldrg.start,ldrg.stop,ld.minStep()) );
	}
    }
    else
    {
	fhss.setToAll( false );
	CubeData cd; cd.copyContents( *this );
	pos_steprg_type inlrg, crlrg;
	cd.getInlRange( inlrg ); cd.getCrlRange( crlrg );
	fhss.cubeHorSubSel().inlSubSel().setInputPosRange( inlrg );
	fhss.cubeHorSubSel().crlSubSel().setInputPosRange( crlrg );
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
	if ( linenr == 0 )
	    continue;

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
{ return includes( bid.inl(), bid.crl() ); }
bool PosInfo::LineCollData::includes( const Bin2D& b2d ) const
{ return includes( b2d.lineNr(), b2d.trcNr() ); }
bool PosInfo::LineCollData::includes( const TrcKey& tk ) const
{ return includes( tk.lineNr(), tk.trcNr() ); }


bool PosInfo::LineCollData::includes( pos_type lnr, pos_type tnr ) const
{
    idx_type lidx = lineIndexOf( lnr );
    if ( lidx < 0 )
	return false;

    const auto& segs = get( lidx )->segments_;
    for ( idx_type iseg=0; iseg<segs.size(); iseg++ )
    {
	const auto& seg = segs[iseg];
	if ( seg.includes(tnr,false) )
	    return seg.step<2 ? true : (tnr-seg.start) % seg.step == 0;
    }

    return false;
}


bool PosInfo::LineCollData::isValid( const LineCollDataPos& lcdp ) const
{
    if ( lcdp.lidx_ < 0 || lcdp.lidx_ >= size() )
	return false;
    const auto& segs( get(lcdp.lidx_)->segments_ );
    if ( lcdp.segnr_ < 0 || lcdp.segnr_ >= segs.size() )
	return false;
    return lcdp.sidx_ >= 0 && lcdp.sidx_ <= segs[lcdp.segnr_].nrSteps();
}


bool PosInfo::LineCollData::hasPosition( const Survey::HorSubSel& hss,
					    glob_size_type idx ) const
{
    if ( isCubeData() )
    {
	const CubeData* cd = asCubeData();
	const CubeHorSubSel* chss = hss.asCubeHorSubSel();
	if ( chss )
	    return cd->hasPosition( *chss, idx );
    }
    else
    {
	const LinesData* ld = asLinesData();
	const LineHorSubSel* lhss = hss.asLineHorSubSel();
	if ( lhss )
	    return ld->hasPosition( *lhss, idx );
    }

    return false;
}


bool PosInfo::LineCollData::toNext( LineCollDataPos& lcdp ) const
{
    if ( lcdp.lidx_ < 0 || lcdp.lidx_ >= size() )
    {
	lcdp.toStart();
	return isValid(lcdp);
    }
    else if ( lcdp.segnr_ < 0 )
	lcdp.segnr_ = lcdp.sidx_ = 0;
    else
    {
	const auto& segset = get(lcdp.lidx_)->segments_;
	lcdp.sidx_++;
	if ( lcdp.sidx_ > segset.get(lcdp.segnr_).nrSteps() )
	{
	    lcdp.segnr_++; lcdp.sidx_ = 0;
	    if ( lcdp.segnr_ >= segset.size() )
	    {
		lcdp.lidx_++; lcdp.segnr_ = 0;
		if ( lcdp.lidx_ >= size() )
		    return false;
	    }
	}
    }
    return true;
}


bool PosInfo::LineCollData::toNextLine( LineCollDataPos& lcdp ) const
{
    lcdp.lidx_++;
    if ( lcdp.lidx_ >= size() )
	return false;

    lcdp.segnr_ = lcdp.sidx_ = 0;
    return true;
}


bool PosInfo::LineCollData::toPrev( LineCollDataPos& lcdp ) const
{
    if ( !isValid(lcdp) )
    {
	if ( isEmpty() )
	    return false;

	lcdp.lidx_ = size() - 1;
	const auto& segs = get(lcdp.lidx_)->segments_;
	lcdp.segnr_ = segs.size() - 1;
	lcdp.sidx_ = segs[lcdp.segnr_].nrSteps();
	return true;
    }
    else
    {
	lcdp.sidx_--;
	if ( lcdp.sidx_ < 0 )
	{
	    lcdp.segnr_--;
	    if ( lcdp.segnr_ < 0 )
	    {
		lcdp.lidx_--;
		if ( lcdp.lidx_ < 0 )
		    return false;
		lcdp.segnr_ = get(lcdp.lidx_)->segments_.size() - 1;
	    }
	    lcdp.sidx_ = get(lcdp.lidx_)->segments_.get(lcdp.segnr_).nrSteps();
	}
	return true;
    }
}


PosInfo::LineCollData::pos_type
PosInfo::LineCollData::lineNr( const LineCollDataPos& lcdp ) const
{
    return !isValid(lcdp) ? 0 : get(lcdp.lidx_)->linenr_;
}


PosInfo::LineCollData::pos_type
PosInfo::LineCollData::trcNr( const LineCollDataPos& lcdp ) const
{
    return !isValid(lcdp) ? 0
	 : get(lcdp.lidx_)->segments_.get(lcdp.segnr_).atIndex(lcdp.sidx_);
}


BinID PosInfo::LineCollData::binID( const LineCollDataPos& lcdp ) const
{
    const auto tnr = trcNr( lcdp );
    return tnr ? BinID( get(lcdp.lidx_)->linenr_, tnr ) : BinID(0,0);
}


Bin2D PosInfo::LineCollData::bin2D( const LineCollDataPos& lcdp ) const
{
    const auto tnr = trcNr( lcdp );
    const auto gid = tnr ? GeomID( get(lcdp.lidx_)->linenr_ ) : GeomID();
    return Bin2D( gid, tnr );
}


PosInfo::LineCollDataPos PosInfo::LineCollData::lineCollPos( pos_type lnr,
							pos_type trcnr ) const
{
    LineCollDataPos lcdp;
    lcdp.lidx_ = lineIndexOf( lnr );
    if ( lcdp.lidx_ < 0 )
	return lcdp;
    const auto& segs( get(lcdp.lidx_)->segments_ );
    for ( idx_type iseg=0; iseg<segs.size(); iseg++ )
    {
	const auto& seg( segs[iseg] );
	if ( seg.includes(trcnr,true) )
	{
	    if ( !seg.step || !((trcnr-seg.start) % seg.step) )
	    {
		lcdp.segnr_ = iseg;
		lcdp.sidx_ = seg.getIndex( trcnr );
	    }
	    break;
	}
    }
    return lcdp;
}


PosInfo::LineCollDataPos PosInfo::LineCollData::lineCollPos(
						const BinID& bid ) const
{ return lineCollPos( bid.inl(), bid.crl() ); }
PosInfo::LineCollDataPos PosInfo::LineCollData::lineCollPos(
						const Bin2D& b2d ) const
{ return lineCollPos( b2d.lineNr(), b2d.trcNr() ); }
PosInfo::LineCollDataPos PosInfo::LineCollData::lineCollPos(
						const TrcKey& tk ) const
{ return lineCollPos( tk.lineNr(), tk.trcNr() ); }


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


PosInfo::LineDataFiller& PosInfo::LineDataFiller::add( const pos_type* posarr,
						       size_type sz )
{
    for ( auto idx=0; idx<sz; idx++ )
	add( posarr[idx] );
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
