/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsinfo.cc,v 1.2 2001-05-02 13:50:23 windev Exp $";

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
	compinfo += new CBVSComponentInfo( *ci.compinfo[idx] );

    return *this;
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


CBVSInfo::SurvGeom::InlineInfo* CBVSInfo::SurvGeom::getInfoFor( int inl )
{
    for ( int idx=0; idx<inldata.size(); idx++ )
	if ( inldata[idx]->inl == inl ) return inldata[idx];
    return 0;
}
