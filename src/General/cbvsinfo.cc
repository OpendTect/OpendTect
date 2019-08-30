/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/


#include "cbvsinfo.h"
#include "trckeyzsampling.h"
#include <iostream>


CBVSInfo& CBVSInfo::operator =( const CBVSInfo& ci )
{
    seqnr_ = ci.seqnr_;
    nrtrcsperposn_ = ci.nrtrcsperposn_;
    auxinfosel_ = ci.auxinfosel_;
    geom_ = ci.geom_;
    stdtext_ = ci.stdtext_;
    usertext_ = ci.usertext_;
    sd_ = ci.sd_;
    nrsamples_ = ci.nrsamples_;

    deepErase( compinfo_ );
    for ( int idx=0; idx<ci.compinfo_.size(); idx++ )
	compinfo_ += new BasicComponentInfo( *ci.compinfo_[idx] );

    return *this;
}


int CBVSInfo::SurvGeom::outOfRange( const BinID& bid ) const
{
    int res = 0;

    if ( bid.inl() < start.inl() || bid.inl() > stop.inl()
	    || (bid.inl()-start.inl())%step.inl() )
	    res = 2;
    if ( bid.crl() < start.crl() || bid.crl() > stop.crl()
	    || (bid.crl()-start.crl())%step.crl() )
	    res += 2 * 256;

    return res;
}


int CBVSInfo::SurvGeom::excludes( const BinID& bid ) const
{
    int res = outOfRange( bid );
    if ( res ) return res;

    return cubedata.includes(bid) ? 0 : 1 + 256;
}


bool CBVSInfo::SurvGeom::includesInline( int inl ) const
{
    if ( fullyrectandreg )
    {
	inl -= start.inl();
	return inl >= 0 &&
		inl + start.inl() <= stop.inl() &&
		inl % step.inl() == 0;
    }

    return cubedata.includesLine( inl );
}


#define nrsameposretries 100
bool CBVSInfo::SurvGeom::moveToNextPos( BinID& bid ) const
{
    PosInfo::LineCollDataPos lcdp( cubedata.lineCollPos(bid) );
    if ( !lcdp.isValid() )
	lcdp.toPreStart();

    const BinID initialbid( bid );
    int nrretry = 0;
    while ( bid == initialbid && nrretry < nrsameposretries )
    {
	nrretry++;
	if ( !cubedata.toNext(lcdp) )
	    return false;

	bid = cubedata.binID( lcdp );
    }

    return bid != initialbid;
}


bool CBVSInfo::SurvGeom::moveToNextInline( BinID& bid ) const
{
    PosInfo::LineCollDataPos lcdp( cubedata.lineCollPos(bid) );
    if ( !lcdp.isValid() )
	{ lcdp.toPreStart(); return moveToNextPos( bid ); }

    lcdp.lidx_++; lcdp.segnr_ = lcdp.sidx_ = 0;
    if ( !cubedata.isValid(lcdp) )
	return false;

    bid = cubedata.binID( lcdp );
    return true;
}


void CBVSInfo::SurvGeom::merge( const CBVSInfo::SurvGeom& geom )
{
    if ( !geom.fullyrectandreg || !fullyrectandreg
      || start.crl() != geom.start.crl() || stop.crl() != geom.stop.crl()
      || step.crl() != geom.step.crl() || step.inl() != geom.step.inl() )
        { mergeIrreg( geom ); return; }

    int expected_inline = stop.inl() + abs(step.inl());
    bool isafter = true;
    if ( geom.start.inl() != expected_inline )
    {
        expected_inline = start.inl() - abs(step.inl());
        if ( geom.stop.inl() == expected_inline )
            isafter = false;
        else
            { mergeIrreg( geom ); return; }
    }

    if ( isafter ) stop.inl() = geom.stop.inl();
    else           start.inl() = geom.start.inl();
}


void CBVSInfo::SurvGeom::toIrreg()
{
    if ( !fullyrectandreg ) return;

    deepErase( cubedata );
    fullyrectandreg = false;
    const int nrinls = (stop.inl()-start.inl())/ abs(step.inl()) + 1;
    const int startinl = step.inl() > 0 ? start.inl() : stop.inl();
    for ( int idx=0; idx<nrinls; idx++ )
    {
	int curinl = startinl + idx * step.inl();
	PosInfo::LineData* newinf = new PosInfo::LineData( curinl );
	newinf->segments_ += PosInfo::LineData::Segment(
				step.crl() > 0 ? start.crl() : stop.crl(),
				step.crl() > 0 ? stop.crl() : start.crl(),
				step.crl() );
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
	const int myidx = cubedata.lineIndexOf( gii->linenr_ );
	PosInfo::LineData* ii = myidx >= 0 ? cubedata[myidx] : 0;
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

    TrcKeySampling hs(false);
    hs.start_ = start; hs.stop_ = stop;
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
		hs.start_ = hs.stop_ = BinID( ii.linenr_, seg.start );
		firstpos = false;
	    }
	    hs.include( BinID(ii.linenr_,seg.start) );
	    hs.include( BinID(ii.linenr_,seg.stop) );
	}
    }

    start = hs.start_;
    stop = hs.stop_;
}


bool CBVSInfo::contributesTo( const TrcKeyZSampling& cs ) const
{
    if ( cs.hsamp_.start_.inl() > geom_.stop.inl() ||
	 cs.hsamp_.stop_.inl() < geom_.start.inl() ||
	 cs.hsamp_.start_.crl() > geom_.stop.crl() ||
	 cs.hsamp_.stop_.crl() < geom_.start.crl() )
	return false;

    float zend = sd_.start + (nrsamples_-1) * sd_.step;
    if ( sd_.start > cs.zsamp_.stop+1e-7 || zend < cs.zsamp_.start-1e-7 )
	return false;

    return true;
}
