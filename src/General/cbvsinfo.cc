/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsinfo.cc,v 1.4 2001-06-01 22:31:59 bert Exp $";

#include "cbvsinfo.h"
#include "binidselimpl.h"


CBVSInfo::SurvGeom& CBVSInfo::SurvGeom::operator =(
			const CBVSInfo::SurvGeom& sg )
{
    fullyrectandreg = sg.fullyrectandreg;
    start = sg.start;
    stop = sg.stop;
    step = sg.step;
    b2c = sg.b2c;

    for ( int idx=0; idx<sg.inldata.size(); idx++ )
	inldata += new CBVSInfo::SurvGeom::InlineInfo( *sg.inldata[idx] );

    return *this;
}


CBVSInfo& CBVSInfo::operator =( const CBVSInfo& ci )
{
    seqnr = ci.seqnr;
    nrtrcsperposn = ci.nrtrcsperposn;
    explinfo = ci.explinfo;
    geom = ci.geom;
    stdtext = ci.stdtext;
    usertext = ci.usertext;

    for ( int idx=0; idx<ci.compinfo.size(); idx++ )
	compinfo += new BasicComponentInfo( *ci.compinfo[idx] );

    return *this;
}


int CBVSInfo::SurvGeom::outOfRange( const BinID& bid ) const
{
    int res = 0;

    if ( step.inl > 0 )
	{ if ( bid.inl < start.inl || bid.inl > stop.inl
	    || (bid.inl-start.inl)%step.inl )
	    res = 2; }
    else
	{ if ( bid.inl > start.inl || bid.inl < stop.inl
	    || (start.inl-bid.inl)%(-step.inl) )
	    res = 2; }
    if ( step.crl > 0 )
	{ if ( bid.crl < start.crl || bid.crl > stop.crl
	    || (bid.crl-start.crl)%step.crl )
	    res += 2 * 256; }
    else
	{ if ( bid.crl > start.crl || bid.crl < stop.crl
	    || (start.crl-bid.crl)%(-step.crl) )
	    res += 2 * 256; }

    return res;
}


int CBVSInfo::SurvGeom::getInfIdx( const BinID& bid, int& infidx ) const
{
    int res = outOfRange( bid );
    if ( fullyrectandreg || res % 256 ) return res;

    infidx = getInfoIdxFor( bid.inl );
    return infidx < 0 ? 2 : res;
}


int CBVSInfo::SurvGeom::excludes( const BinID& bid ) const
{
    int infidx = -1;
    int res = getInfIdx( bid, infidx );
    if ( infidx < 0 ) return res;

    InlineInfo inlinf = *inldata[infidx];
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
	infidx++;
	if ( infidx >= inldata.size() ) return false;
	bid.inl = inldata[infidx]->inl;
	bid.crl = inldata[infidx]->segments[0].start;
	return true;
    }
    else if ( fullyrectandreg )
    {
	bid.crl = start.crl;
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

    const InlineInfo& inlinf = *inldata[infidx];
    if ( inlinf.segments.size() == 1 )
	return toNextInline( bid );

    int iseg = -1;
    for ( int idx=0; idx<inlinf.segments.size(); idx++ )
    {
	CBVSInfo::SurvGeom::InlineInfo::Segment& seg = inlinf.segments[idx];
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

    deepErase( inldata );
    fullyrectandreg = false;
    const int nrinls = (stop.inl-start.inl)/step.inl + 1;
    for ( int idx=0; idx<nrinls; idx++ )
    {
	int curinl = start.inl + idx * step.inl;
	CBVSInfo::SurvGeom::InlineInfo* newinf
	    = new CBVSInfo::SurvGeom::InlineInfo( curinl );
	newinf->segments += CBVSInfo::SurvGeom::InlineInfo::Segment(
				start.crl, stop.crl, step.crl );
	inldata += newinf;
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

    for ( int idx=0; idx<geom->inldata.size(); idx++ )
    {
	const CBVSInfo::SurvGeom::InlineInfo* gii = geom->inldata[idx];
	CBVSInfo::SurvGeom::InlineInfo* ii = getInfoFor( gii->inl );
	if ( !ii )
	    inldata += new CBVSInfo::SurvGeom::InlineInfo( *gii );
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
    for ( int idx=0; idx<inldata.size(); idx++ )
    {
	const CBVSInfo::SurvGeom::InlineInfo& ii = *inldata[idx];
	for ( int iseg=0; iseg<ii.segments.size(); iseg++ )
	{
	    const CBVSInfo::SurvGeom::InlineInfo::Segment& seg
				= ii.segments[iseg];
	    bidrg.include( BinID(ii.inl,seg.start) );
	    bidrg.include( BinID(ii.inl,seg.stop) );
	}
    }

    start = bidrg.start;
    stop = bidrg.stop;
}


int CBVSInfo::SurvGeom::getInfoIdxFor( int inl ) const
{
    for ( int idx=0; idx<inldata.size(); idx++ )
	if ( inldata[idx]->inl == inl ) return idx;
    return -1;
}


CBVSInfo::SurvGeom::InlineInfo* CBVSInfo::SurvGeom::getInfoFor( int inl )
{
    int idx = getInfoIdxFor( inl );
    return idx < 0 ? 0 : inldata[idx];
}
