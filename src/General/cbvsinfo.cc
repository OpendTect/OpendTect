/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsinfo.cc,v 1.25 2009-07-22 16:01:32 cvsbert Exp $";

#include "cbvsinfo.h"
#include "cubesampling.h"
#include <iostream>


CBVSInfo& CBVSInfo::operator =( const CBVSInfo& ci )
{
    seqnr = ci.seqnr;
    nrtrcsperposn = ci.nrtrcsperposn;
    auxinfosel = ci.auxinfosel;
    geom = ci.geom;
    stdtext = ci.stdtext;
    usertext = ci.usertext;
    sd = ci.sd;
    nrsamples = ci.nrsamples;

    deepErase( compinfo );
    for ( int idx=0; idx<ci.compinfo.size(); idx++ )
	compinfo += new BasicComponentInfo( *ci.compinfo[idx] );

    return *this;
}


int CBVSInfo::SurvGeom::outOfRange( const BinID& bid ) const
{
    int res = 0;

    if ( bid.inl < start.inl || bid.inl > stop.inl
	    || (bid.inl-start.inl)%step.inl )
	    res = 2;
    if ( bid.crl < start.crl || bid.crl > stop.crl
	    || (bid.crl-start.crl)%step.crl )
	    res += 2 * 256;

    return res;
}


int CBVSInfo::SurvGeom::getInfIdx( const BinID& bid, int& infidx ) const
{
    int res = outOfRange( bid );
    if ( fullyrectandreg || res % 256 ) return res;

    infidx = cubedata.indexOf( bid.inl );
    return infidx < 0 ? 2 : res;
}


bool CBVSInfo::SurvGeom::includesInline( int inl ) const
{
    if ( fullyrectandreg )
    {
	inl -= start.inl;
	return inl >= 0 && inl + start.inl <= stop.inl && inl % step.inl == 0;
    }

    return getInfoFor(inl) ? true : false;
}


int CBVSInfo::SurvGeom::excludes( const BinID& bid ) const
{
    int infidx = -1;
    int res = getInfIdx( bid, infidx );
    if ( infidx < 0 ) return res;

    PosInfo::LineData inlinf = *cubedata[infidx];
    for ( int idx=0; idx<inlinf.segments_.size(); idx++ )
    {
	if ( inlinf.segments_[idx].includes(bid.crl) )
	    return 0;
    }
    return 1 + 256;
}


int CBVSInfo::SurvGeom::findNextInfIdx( int curinlinfnr ) const
{
    const bool inlrev = step.inl < 0;
    if ( curinlinfnr < 0 || curinlinfnr >= cubedata.size() )
	return inlrev ? cubedata.size()-1 : 0;

    const int curinl = cubedata[curinlinfnr]->linenr_;
    int inlinfnr = curinlinfnr + (inlrev ? -1 : 1);
    // Try the next one first - most probably that's the one we need
    if ( inlinfnr >= 0 && inlinfnr < cubedata.size() )
    {
	int inldiff = cubedata[inlinfnr]->linenr_ - curinl;
	if ( inldiff > 0 && inldiff <= abs(step.inl) )
	    return inlinfnr;
    }

    // Nope. We need to find the nearest higher inline number
    inlinfnr = -1; int mindiff = mUdf(int);
    for ( int idx=0; idx<cubedata.size(); idx++ )
    {
	int inldiff = cubedata[idx]->linenr_ - curinl;
	if ( inldiff > 0 && inldiff < mindiff )
	    { mindiff = inldiff; inlinfnr = idx; }
    }

    return inlinfnr;

}


bool CBVSInfo::SurvGeom::toNextInline( BinID& bid ) const
{
    int infidx = -1;
    int res = getInfIdx( bid, infidx );
    if ( infidx >= 0 )
    {
	infidx = findNextInfIdx( infidx );
	if ( infidx < 0 )
	    return false;
	bid.inl = cubedata[infidx]->linenr_;
	bid.crl = cubedata[infidx]->segments_[0].start;
	return true;
    }
    else if ( fullyrectandreg )
    {
	bid.inl += step.inl < 0 ? -step.inl : step.inl;
	bid.crl = step.crl < 0 ? stop.crl : start.crl;
	return !outOfRange( bid );
    }

    return false;
}


bool CBVSInfo::SurvGeom::toNextBinID( BinID& bid ) const
{
    bid.crl += step.crl;
    int infidx = -1;
    int res = getInfIdx( bid, infidx );
    if ( !res )
	return true;

    if ( fullyrectandreg )
	return toNextInline( bid );
    else if ( infidx < 0 )
	return false;

    const PosInfo::LineData& inlinf = *cubedata[infidx];
    if ( inlinf.segments_.size() == 1 )
	return toNextInline( bid );

    int iseg = -1;
    for ( int idx=0; idx<inlinf.segments_.size(); idx++ )
    {
	const PosInfo::LineData::Segment& seg = inlinf.segments_[idx];
	if ( (seg.step > 0 && seg.start > bid.crl)
	  || (seg.step < 0 && seg.start < bid.crl) )
	    { iseg = idx; break; }
    }
    if ( iseg < 0 || iseg == inlinf.segments_.size()-1 )
	return toNextInline( bid );

    bid.crl = inlinf.segments_[iseg+1].start;
    return true;
}


void CBVSInfo::SurvGeom::merge( const CBVSInfo::SurvGeom& geom )
{
    if ( !geom.fullyrectandreg || !fullyrectandreg
      || start.crl != geom.start.crl || stop.crl != geom.stop.crl
      || step.crl != geom.step.crl || step.inl != geom.step.inl )
        { mergeIrreg( geom ); return; }

    int expected_inline = stop.inl + abs(step.inl);
    bool isafter = true;
    if ( geom.start.inl != expected_inline )
    {
        expected_inline = start.inl - abs(step.inl);
        if ( geom.stop.inl == expected_inline )
            isafter = false;
        else
            { mergeIrreg( geom ); return; }
    }

    if ( isafter ) stop.inl = geom.stop.inl;
    else           start.inl = geom.start.inl;
}


void CBVSInfo::SurvGeom::toIrreg()
{
    if ( !fullyrectandreg ) return;

    deepErase( cubedata );
    fullyrectandreg = false;
    const int nrinls = (stop.inl-start.inl)/ abs(step.inl) + 1;
    const int startinl = step.inl > 0 ? start.inl : stop.inl;
    for ( int idx=0; idx<nrinls; idx++ )
    {
	int curinl = startinl + idx * step.inl;
	PosInfo::LineData* newinf = new PosInfo::LineData( curinl );
	newinf->segments_ += PosInfo::LineData::Segment(
				step.crl > 0 ? start.crl : stop.crl,
				step.crl > 0 ? stop.crl : start.crl,
				step.crl );
	cubedata.add( newinf );
    }
}


void CBVSInfo::SurvGeom::mergeIrreg( const CBVSInfo::SurvGeom& g )
{
    const CBVSInfo::SurvGeom* geom = &g;
    if ( geom->fullyrectandreg )
    {
	geom = new CBVSInfo::SurvGeom( g );
	const_cast<CBVSInfo::SurvGeom*>(geom)->toIrreg();
    }
    toIrreg();

    for ( int idx=0; idx<geom->cubedata.size(); idx++ )
    {
	const PosInfo::LineData* gii = geom->cubedata[idx];
	PosInfo::LineData* ii = getInfoFor( gii->linenr_ );
	if ( !ii )
	    cubedata.add( new PosInfo::LineData( *gii ) );
	else
	{
	    // not correct in case of overlap
	    // but that is asking a bit much, really
	    for ( int iseg=0; iseg<gii->segments_.size(); iseg++ )
		ii->segments_ += gii->segments_[idx];
	}
    }

    reCalcBounds();
    if ( geom != &g )
	delete const_cast<CBVSInfo::SurvGeom*>( geom );
}


void CBVSInfo::SurvGeom::reCalcBounds()
{
    if ( fullyrectandreg ) return;

    HorSampling hs(false);
    hs.start = start; hs.stop = stop;
    bool firstpos = true;
    for ( int icd=0; icd<cubedata.size(); icd++ )
    {
	const PosInfo::LineData& ii = *cubedata[icd];
	for ( int iseg=0; iseg<ii.segments_.size(); iseg++ )
	{
	    const PosInfo::LineData::Segment& seg = ii.segments_[iseg];
	    if ( !seg.start && !seg.stop )
	    {
#ifdef __debug__
		std::cerr << "CBVSInfo - Empty segment: " << ii.linenr_
		    	  << " segment " << iseg << std::endl;
#endif
		continue;
	    }

	    if ( firstpos )
	    {
		hs.start = hs.stop = BinID( ii.linenr_, seg.start );
		firstpos = false;
	    }
	    hs.include( BinID(ii.linenr_,seg.start) );
	    hs.include( BinID(ii.linenr_,seg.stop) );
	}
    }

    start = hs.start;
    stop = hs.stop;
}


PosInfo::LineData* CBVSInfo::SurvGeom::gtInfFor( int inl ) const
{
    const int idx = cubedata.indexOf( inl );
    return idx < 0 ? 0 : const_cast<PosInfo::LineData*>(cubedata[idx]);
}


bool CBVSInfo::contributesTo( const CubeSampling& cs ) const
{
    if ( cs.hrg.start.inl > geom.stop.inl || cs.hrg.stop.inl < geom.start.inl
      || cs.hrg.start.crl > geom.stop.crl || cs.hrg.stop.crl < geom.start.crl )
	return false;

    float zend = sd.start + (nrsamples-1) * sd.step;
    if ( sd.start > cs.zrg.stop+1e-7 || zend < cs.zrg.start-1e-7 )
	return false;

    return true;
}
