/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/

static const char* rcsID = "$Id: posinfo.cc,v 1.1 2008-03-12 09:48:03 cvsbert Exp $";

#include "posinfo.h"
#include "survinfo.h"
#include "cubesampling.h"


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

    int ret = 0; float mindist = 1e30;
    for ( int iseg=0; iseg<segments_.size(); iseg++ )
    {
	const PosInfo::LineData::Segment& seg = segments_[iseg];

	const bool isrev = seg.step < 0;
	const float hstep = seg.step * 0.5;
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



PosInfo::CubeData& PosInfo::CubeData::operator =( const PosInfo::CubeData& cd )
{
    if ( &cd != this )
    {
	deepErase( *this );
	for ( int idx=0; idx<cd.size(); idx++ )
	    *this += new PosInfo::LineData( *cd[idx] );
    }
    return *this;
}


int PosInfo::CubeData::indexOf( int lnr ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->linenr_ == lnr )
	    return idx;
    return -1;
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
	const PosInfo::LineData& ld = *(*this)[0];
	for ( int icrl=0; icrl<ld.segments_.size(); icrl++ )
	{
	    const PosInfo::LineData::Segment& seg = ld.segments_[icrl];
	    if ( seg.start != seg.stop )
		return true;
	}
    }

    return false;
}


PosInfo::Line2DData::Line2DData()
{
    zrg = SI().sampling(false).zrg;
}


void PosInfo::Line2DData::dump( std::ostream& strm, bool pretty ) const
{
    if ( !pretty )
	strm << zrg.start << '\t' << zrg.stop << '\t' << zrg.step << std::endl;
    else
    {
	const float fac = SI().zFactor();
	strm << "Z range " << SI().getZUnit() << ":\t" << fac*zrg.start
	     << '\t' << fac*zrg.stop << "\t" << fac*zrg.step;
	strm << "\n\nTrace number\tX-coord\tY-coord" << std::endl;
    }

    for ( int idx=0; idx<posns.size(); idx++ )
    {
	const PosInfo::Line2DPos& pos = posns[idx];
	strm << pos.nr_ << '\t' << pos.coord_.x << '\t' << pos.coord_.y << '\n';
    }
    strm.flush();
}
