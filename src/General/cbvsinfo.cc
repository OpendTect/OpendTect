/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsinfo.cc,v 1.17 2005-01-05 15:06:57 bert Exp $";

#include "cbvsinfo.h"
#include "binidselimpl.h"
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

    PosInfo::InlData inlinf = *cubedata[infidx];
    for ( int idx=0; idx<inlinf.segments.size(); idx++ )
    {
	if ( inlinf.segments[idx].includes(bid.crl) )
	    return 0;
    }
    return 1 + 256;
}


bool CBVSInfo::SurvGeom::toNextInline( BinID& bid ) const
{
    int infidx = -1;
    int res = getInfIdx( bid, infidx );
    if ( infidx >= 0 )
    {
	while ( cubedata[infidx]->inl == bid.inl )
	{
	    infidx++;
	    if ( infidx >= cubedata.size() ) return false;
	}

	bid.inl = cubedata[infidx]->inl;
	bid.crl = cubedata[infidx]->segments[0].start;
	return true;
    }
    else if ( fullyrectandreg )
    {
	bid.crl = step.crl > 0 ? start.crl : stop.crl;
	bid.inl += step.inl;
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

    const PosInfo::InlData& inlinf = *cubedata[infidx];
    if ( inlinf.segments.size() == 1 )
	return toNextInline( bid );

    int iseg = -1;
    for ( int idx=0; idx<inlinf.segments.size(); idx++ )
    {
	PosInfo::InlData::Segment& seg = inlinf.segments[idx];
	if ( (seg.step > 0 && seg.start > bid.crl)
	  || (seg.step < 0 && seg.start < bid.crl) )
	    { iseg = idx; break; }
    }
    if ( iseg < 0 || iseg == inlinf.segments.size()-1 )
	return toNextInline( bid );

    bid.crl = inlinf.segments[iseg+1].start;
    return true;
}


void CBVSInfo::SurvGeom::merge( const CBVSInfo::SurvGeom& geom )
{
    if ( !geom.fullyrectandreg || !fullyrectandreg )
        { mergeIrreg( geom ); return; }

    int expected_inline = stop.inl + step.inl;
    bool isafter = true;
    if ( geom.start.inl != expected_inline )
    {
        expected_inline = start.inl - step.inl;
        if ( geom.stop.inl == expected_inline )
            isafter = false;
        else
            { mergeIrreg( geom ); return; }
    }

    if ( isafter ) stop.inl = geom.stop.inl;
    else           start.inl = geom.start.inl;

    StepInterval<int> crls( start.crl, stop.crl, step.crl );
    crls.include( geom.start.crl );
    crls.include( geom.stop.crl );
    start.crl = crls.start;
    stop.crl = crls.stop;
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
	PosInfo::InlData* newinf = new PosInfo::InlData( curinl );
	newinf->segments += PosInfo::InlData::Segment(
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
	const PosInfo::InlData* gii = geom->cubedata[idx];
	PosInfo::InlData* ii = getInfoFor( gii->inl );
	if ( !ii )
	    cubedata.add( new PosInfo::InlData( *gii ) );
	else
	{
	    //TODO make correct merging of segments in case of overlap
	    for ( int iseg=0; iseg<gii->segments.size(); iseg++ )
		ii->segments += gii->segments[idx];
	}
    }

    reCalcBounds();
    if ( geom != &g )
	delete const_cast<CBVSInfo::SurvGeom*>( geom );
}


void CBVSInfo::SurvGeom::reCalcBounds()
{
    if ( fullyrectandreg ) return;

    BinIDRange bidrg;
    bidrg.start = start; bidrg.stop = stop;
    for ( int idx=0; idx<cubedata.size(); idx++ )
    {
	const PosInfo::InlData& ii = *cubedata[idx];
	for ( int iseg=0; iseg<ii.segments.size(); iseg++ )
	{
	    const PosInfo::InlData::Segment& seg = ii.segments[iseg];
	    if ( !seg.start && !seg.stop )
	    {
#ifdef __debug__
		std::cerr << "Empty segment: " << ii.inl
		    	  << " segment " << iseg << std::endl;
#endif
		continue;
	    }

	    bidrg.include( BinID(ii.inl,seg.start) );
	    bidrg.include( BinID(ii.inl,seg.stop) );
	}
    }

    start = bidrg.start;
    stop = bidrg.stop;
}


PosInfo::InlData* CBVSInfo::SurvGeom::gtInfFor( int inl ) const
{
    const int idx = cubedata.indexOf( inl );
    return idx < 0 ? 0 : const_cast<PosInfo::InlData*>(cubedata[idx]);
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
