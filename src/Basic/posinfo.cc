/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/

static const char* rcsID = "$Id: posinfo.cc,v 1.26 2010-12-03 20:41:27 cvskris Exp $";

#include "posinfo.h"
#include "survinfo.h"
#include "math2.h"
#include <iostream>


int PosInfo::LineData::size() const
{
    int res = 0;
    for ( int idx=0; idx<segments_.size(); idx++ )
	res += segments_[idx].nrSteps() + 1;
    return res;
}


int PosInfo::LineData::nearestSegment( double x ) const
{
    if ( segments_.size() < 1 )
	return -1;

    int ret = 0; float mindist = mUdf(float);
    for ( int iseg=0; iseg<segments_.size(); iseg++ )
    {
	const PosInfo::LineData::Segment& seg = segments_[iseg];

	const bool isrev = seg.step < 0;
	const float hstep = (float)seg.step * 0.5f;
	float dist;
	if ( (isrev && x > seg.start+hstep) || (!isrev && x < seg.start-hstep) )
	    dist = x - seg.start;
	else if ( (isrev && x<seg.stop-hstep) || (!isrev && x>seg.stop+hstep))
	    dist = x - seg.stop;
	else
	    { ret = iseg; break; }

	if ( dist < 0 ) dist = -dist;
	if ( dist < mindist )
	    { ret = iseg; mindist = dist; }
    }

    return ret;
}


IndexInfo PosInfo::LineData::getIndexInfo( double x ) const
{
    const int seg = nearestSegment( x );
    if ( seg < 0 )
	return IndexInfo(-1,true,true);

    IndexInfo ret( segments_[seg], x );
    for ( int iseg=0; iseg<seg; iseg++ )
	ret.nearest_ += segments_[iseg].nrSteps() + 1;

    return ret;
}


int PosInfo::LineData::segmentOf( int nr ) const
{
    for ( int iseg=0; iseg<segments_.size(); iseg++ )
    {
	if ( segments_[iseg].includes(nr) )
	    return !((nr-segments_[iseg].start) % segments_[iseg].step);
    }

    return -1;
}


Interval<int> PosInfo::LineData::range() const
{
    if ( segments_.isEmpty() ) return Interval<int>( mUdf(int), mUdf(int) );

    Interval<int> ret( segments_[0].start, segments_[0].start );
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

    Interval<int> rg( ld1.range() ); rg.include( ld2.range() );
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


PosInfo::CubeData::CubeData( const BinID& b1, const BinID& b2,
			     const BinID& stp )
	: ManagedObjectSet<LineData>( false )
{
    BinID start( b1 ); BinID stop( b2 ); BinID step( stp );
    if ( start.inl > stop.inl ) Swap( start.inl, stop.inl );
    if ( start.crl > stop.crl ) Swap( start.crl, stop.crl );
    if ( step.inl < 0 ) step.inl = -step.inl;
    if ( step.crl < 0 ) step.crl = -step.crl;

    for ( int iln=start.inl; iln<=stop.inl; iln+=step.inl )
    {
	LineData* ld = new LineData( iln );
	ld->segments_ += LineData::Segment( start.crl, stop.crl, step.crl );
	add( ld );
    }
}


PosInfo::CubeData& PosInfo::CubeData::operator =( const PosInfo::CubeData& cd )
{
    if ( &cd != this )
    {
	erase();
	for ( int idx=0; idx<cd.size(); idx++ )
	    *this += new PosInfo::LineData( *cd[idx] );
    }
    return *this;
}


int PosInfo::CubeData::totalSize() const
{
    int totalsize = 0;
    for ( int idx=0; idx<size(); idx++ )
	totalsize += (*this)[idx]->size();

    return totalsize;
}


int PosInfo::CubeData::totalSizeInside( const HorSampling& hrg ) const
{
    int totalsize = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	const PosInfo::LineData* linedata = (*this)[idx];
	if ( !hrg.inlOK( linedata->linenr_ ) )
	    continue;

	for ( int idy=0; idy<linedata->segments_.size(); idy++ )
	{
	    const PosInfo::LineData::Segment& segment =
		linedata->segments_[idy];

	    for ( int crl=segment.start; crl<=segment.stop; crl+=segment.step )
	    {
		if ( hrg.crlOK(crl) )
		    totalsize ++;
	    }
	}
    }

    return totalsize;
}


int PosInfo::CubeData::indexOf( int lnr ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->linenr_ == lnr )
	    return idx;
    return -1;
}


void PosInfo::CubeData::limitTo( const HorSampling& hsin )
{
    HorSampling hs ( hsin );
    hs.normalise();
    for ( int iidx=size()-1; iidx>=0; iidx-- )
    {
	PosInfo::LineData* ld = (*this)[iidx];
	if ( !hs.inlOK(ld->linenr_) )
	{ ld = remove( iidx ); delete ld; continue; }

	int nrvalidsegs = 0;
	for ( int iseg=ld->segments_.size()-1; iseg>=0; iseg-- )
	{
	    StepInterval<int>& seg = ld->segments_[iseg];
	    if ( seg.start > hs.stop.crl || seg.stop < hs.start.crl )
	    { ld->segments_.remove( iseg ); continue; }

	    seg.step = Math::LCMOf( seg.step, hs.step.crl );
	    if ( !seg.step )
	    { ld->segments_.remove( iseg ); continue; }

	    if ( seg.start < hs.start.crl )
	    {
		int newstart = hs.start.crl;
		int diff = newstart - seg.start;
		if ( diff % seg.step )
		{
		    diff += seg.step - diff % seg.step;
		    newstart = seg.start + diff;
		}

		seg.start = newstart;
	    }
	    if ( seg.stop > hs.stop.crl )
	    {
		int newstop = hs.stop.crl;
		int diff = seg.stop - newstop;
		if ( diff % seg.step )
		{
		    diff += seg.step - diff % seg.step;
		    newstop = seg.stop - diff;
		}

		seg.stop = newstop;
	    }
	    if ( seg.start > seg.stop )
		ld->segments_.remove( iseg );
	    else nrvalidsegs++;
	}

	if ( !nrvalidsegs )
	{ ld = remove( iidx ); delete ld; }
    }
}


void PosInfo::CubeData::sort()
{
    const int sz = size();

    for ( int d=sz/2; d>0; d=d/2 )
	for ( int i=d; i<sz; i++ )
	    for ( int j=i-d;
		    j>=0 && (*this)[j]->linenr_>(*this)[j+d]->linenr_;
		    j-=d )
		swap( j+d, j );
}


bool PosInfo::CubeData::includes( int lnr, int crl ) const
{
    int ilnr = indexOf( lnr ); if ( ilnr < 0 ) return false;
    for ( int iseg=0; iseg<(*this)[ilnr]->segments_.size(); iseg++ )
	if ( (*this)[ilnr]->segments_[iseg].includes(crl) )
	    return true;
    return false;
}


bool PosInfo::CubeData::getInlRange( StepInterval<int>& rg ) const
{
    const int sz = size();
    if ( sz < 1 ) return false;
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

    rg.sort( true );
    return isreg;
}


bool PosInfo::CubeData::getCrlRange( StepInterval<int>& rg ) const
{
    const int sz = size();
    if ( sz < 1 ) return false;

    const PosInfo::LineData* ld = (*this)[0];
    rg = ld->segments_[0];
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

    rg.sort( true );
    return isreg;
}


bool PosInfo::CubeData::isFullyRectAndReg() const
{
    const int sz = size();
    if ( sz < 1 ) return false;

    const PosInfo::LineData* ld = (*this)[0];
    const PosInfo::LineData::Segment seg = ld->segments_[0];

    int lnrstep;
    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	ld = (*this)[ilnr];
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
	    if ( inc ) add( new PosInfo::LineData(ld1) );
	    continue;
	}

	PosInfo::LineData* ld = new PosInfo::LineData( *pd2[iln2] );
	ld->merge( ld1, inc );
	add( ld );
    }
    if ( !inc ) return;

    for ( int iln2=0; iln2<pd2.size(); iln2++ )
    {
	const PosInfo::LineData& ld2 = *pd2[iln2];
	const int iln = indexOf( ld2.linenr_ );
	if ( iln2 < 0 )
	    add( new PosInfo::LineData(ld2) );
    }
}




bool PosInfo::CubeData::read( std::istream& strm, bool asc )
{
    const int intsz = sizeof(int);
    int buf[4]; int itmp = 0;
    if ( asc )
	strm >> itmp;
    else
    {
	strm.read( (char*)buf, intsz );
	itmp = buf[0];
    }
    const int nrinl = itmp;
    if ( nrinl <= 0 ) return false;

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	int linenr = 0, nrseg = 0;
	if ( asc )
	    strm >> linenr >> nrseg;
	else
	{
	    strm.read( (char*)buf, 2 * intsz );
	    linenr = buf[0];
	    nrseg = buf[1];
	}
	if ( linenr == 0 ) continue;

	PosInfo::LineData* iinf = new PosInfo::LineData( linenr );

	PosInfo::LineData::Segment crls;
	for ( int iseg=0; iseg<nrseg; iseg++ )
	{
	    if ( asc )
		strm >> crls.start >> crls.stop >> crls.step;
	    else
	    {
		strm.read( (char*)buf, 3 * intsz );
		crls.start = buf[0];
		crls.stop = buf[1];
		crls.step = buf[2];
	    }
	    iinf->segments_ += crls;
	}

	add( iinf );
    }

    return true;
}


bool PosInfo::CubeData::write( std::ostream& strm, bool asc ) const
{
    const int intsz = sizeof( int );
    const int nrinl = this->size();
    if ( asc )
	strm << nrinl << '\n';
    else
	strm.write( (const char*)&nrinl, intsz );

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	const PosInfo::LineData& inlinf = *(*this)[iinl];
	const int nrcrl = inlinf.segments_.size();
	if ( asc )
	    strm << inlinf.linenr_ << ' ' << nrcrl;
	else
	{
	    strm.write( (const char*)&inlinf.linenr_, intsz );
	    strm.write( (const char*)&nrcrl, intsz );
	}

	for ( int icrl=0; icrl<nrcrl; icrl++ )
	{
	    const PosInfo::LineData::Segment& seg = inlinf.segments_[icrl];
	    if ( asc )
		strm << ' ' << seg.start << ' ' << seg.stop << ' ' << seg.step;
	    else
	    {
		strm.write( (const char*)&seg.start, intsz );
		strm.write( (const char*)&seg.stop, intsz );
		strm.write( (const char*)&seg.step, intsz );
	    }
	    if ( asc )
		strm << '\n';
	}

	if ( !strm.good() ) return false;
    }

    return true;
}


PosInfo::CubeDataFiller::CubeDataFiller( CubeData& cd )
    : cd_(cd)
{
    initLine();
}


PosInfo::CubeDataFiller::~CubeDataFiller()
{
    delete ld_;
}


void PosInfo::CubeDataFiller::add( const BinID& bid )
{
    if ( !ld_ || ld_->linenr_ != bid.inl )
    {
	if ( ld_ )
	    finishLine();
	ld_ = new LineData( bid.inl );
    }

    if ( mIsUdf(prevcrl) )
	prevcrl = seg_.start = seg_.stop = bid.crl;
    else
    {
	const int curstep = bid.crl - prevcrl;
	if ( curstep != 0 )
	{
	    if ( mIsUdf(seg_.step) )
		seg_.step = curstep;
	    else if ( seg_.step != curstep )
	    {
		ld_->segments_ += seg_;
		seg_.start = bid.crl;
		mSetUdf(seg_.step);
	    }
	    prevcrl = seg_.stop = bid.crl;
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
    if ( mIsUdf(seg_.step) )
    {
	if ( ld_->segments_.isEmpty() )
	    seg_.step = SI().crlStep();
	else
	    seg_.step = ld_->segments_[0].step;
    }

    ld_->segments_ += seg_;
    cd_ += ld_;
    initLine();
}


bool PosInfo::CubeDataIterator::next( BinID& bid )
{
    if ( !cubedata_.size() ) return false;

    if ( firstpos_ )
    {
	const PosInfo::LineData* ld = cubedata_[0];
	if ( !ld || !ld->segments_.size() )
	    return false;

	bid.inl = ld->linenr_;
	bid.crl = ld->segments_[0].start;
	firstpos_ = false;
	return true;
    }

    bool startnew = false;
    for ( int idx=0; idx<cubedata_.size(); idx++ )
    {
	const PosInfo::LineData* ld = cubedata_[idx];
	if ( !ld || !ld->segments_.size() )
	    continue;

	if ( !startnew && ld->linenr_ != bid.inl )
	    continue;
	
	bid.inl = ld->linenr_;
	for ( int sdx=0; sdx<ld->segments_.size(); sdx++ )
	{
	    StepInterval<int> crlrg = ld->segments_[sdx];
	    if ( startnew )
	    {
		bid.crl = crlrg.start;	
		return true;
	    }

	    if ( !crlrg.includes(bid.crl) ) continue;

	    bid.crl = bid.crl + crlrg.step;
	    if ( crlrg.includes(bid.crl) )
		return true;
	    else
		startnew = true;
	}
    }

    return false;
}
