/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "cbvsinfo.h"
#include "trckeyzsampling.h"
#include <iostream>

mStartAllowDeprecatedSection

BasicComponentInfo::BasicComponentInfo( const char* nm )
    : NamedObject(nm)
    , datatype_(0)
    , datatype(datatype_)
    , datachar(datachar_)
{}


BasicComponentInfo::BasicComponentInfo( const BasicComponentInfo& ci )
    : NamedObject((const char*)ci.name())
    , datatype(datatype_)
    , datachar(datachar_)
{
    *this = ci;
}

mStopAllowDeprecatedSection


BasicComponentInfo::~BasicComponentInfo()
{}


BasicComponentInfo& BasicComponentInfo::operator=( const BasicComponentInfo& ci)
{
    if ( this == &ci )
	return *this;

    setName( ci.name() );
    datatype_ = ci.datatype_;
    datachar_ = ci.datachar_;
    return *this;
}


bool BasicComponentInfo::operator==( const BasicComponentInfo& ci ) const
{
    return name() == ci.name() &&
	datatype_ == ci.datatype_ &&
	    datachar_ == ci.datachar_;
}


CBVSInfo::CBVSInfo()
{}


CBVSInfo::CBVSInfo( const CBVSInfo& oth )
{
    *this = oth;
}


CBVSInfo::~CBVSInfo()
{
    deepErase(compinfo_);
}


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


int CBVSInfo::estimatedNrTraces() const
{
    return geom_.cubedata_.totalSize() * nrtrcsperposn_;
}


mStartAllowDeprecatedSection

CBVSInfo::SurvGeom::SurvGeom()
    : fullyrectandreg(fullyrectandreg_)
    , start(start_)
    , stop(stop_)
    , step(step_)
    , b2c(b2c_)
    , cubedata(cubedata_)
{}


CBVSInfo::SurvGeom::SurvGeom( const CBVSInfo::SurvGeom& oth )
    : fullyrectandreg(fullyrectandreg_)
    , start(start_)
    , stop(stop_)
    , step(step_)
    , b2c(b2c_)
    , cubedata(cubedata_)
{
    *this = oth;
}

mStopAllowDeprecatedSection


CBVSInfo::SurvGeom::~SurvGeom()
{}


CBVSInfo::SurvGeom&
	CBVSInfo::SurvGeom::operator=( const CBVSInfo::SurvGeom& oth )
{
    fullyrectandreg_ = oth.fullyrectandreg_;
    start_ = oth.start_;
    stop_ = oth.stop_;
    step_ = oth.step_;
    b2c_ = oth.b2c_;
    cubedata_ = oth.cubedata_;

    return *this;
}


int CBVSInfo::SurvGeom::outOfRange( const BinID& bid ) const
{
    int res = 0;

    if ( bid.inl() < start_.inl() || bid.inl() > stop_.inl()
	 || (bid.inl()-start_.inl())%step_.inl() )
	    res = 2;
    if ( bid.crl() < start_.crl() || bid.crl() > stop_.crl()
	 || (bid.crl()-start_.crl())%step_.crl() )
	    res += 2 * 256;

    return res;
}


int CBVSInfo::SurvGeom::excludes( const BinID& bid ) const
{
    int res = outOfRange( bid );
    if ( res ) return res;

    return cubedata_.cubeDataPos(bid).isValid() ? 0 : 1 + 256;
}


bool CBVSInfo::SurvGeom::includesInline( int inl ) const
{
    if ( fullyrectandreg_ )
    {
	inl -= start_.inl();
	return inl >= 0 &&
		inl + start_.inl() <= stop_.inl() &&
		inl % step_.inl() == 0;
    }

    return cubedata_.indexOf(inl) >= 0;
}


void CBVSInfo::SurvGeom::clean()
{
    fullyrectandreg_ = false;
    deepErase(cubedata_);
}


#define nrsameposretries 100
bool CBVSInfo::SurvGeom::moveToNextPos( BinID& bid ) const
{
    PosInfo::CubeDataPos cdp( cubedata_.cubeDataPos(bid) );
    if ( !cdp.isValid() )
	cdp.toPreStart();

    const BinID initialbid( bid );
    int nrretry = 0;
    while ( bid == initialbid && nrretry < nrsameposretries )
    {
	nrretry++;
	if ( !cubedata_.toNext(cdp) )
	    return false;
	bid = cubedata_.binID( cdp );
    }

    return bid != initialbid;
}


bool CBVSInfo::SurvGeom::moveToNextInline( BinID& bid ) const
{
    PosInfo::CubeDataPos cdp( cubedata_.cubeDataPos(bid) );
    if ( !cdp.isValid() )
	{ cdp.toPreStart(); return moveToNextPos( bid ); }

    cdp.lidx_++; cdp.segnr_ = cdp.sidx_ = 0;
    if ( !cubedata_.isValid(cdp) )
	return false;

    bid = cubedata_.binID( cdp );
    return true;
}


void CBVSInfo::SurvGeom::merge( const CBVSInfo::SurvGeom& geom )
{
    if ( !geom.fullyrectandreg_ || !fullyrectandreg_
	 || start_.crl() != geom.start_.crl() || stop_.crl() != geom.stop_.crl()
	 || step_.crl() != geom.step_.crl() || step_.inl() != geom.step_.inl() )
        { mergeIrreg( geom ); return; }

    int expected_inline = stop_.inl() + abs(step_.inl());
    bool isafter = true;
    if ( geom.start_.inl() != expected_inline )
    {
	expected_inline = start_.inl() - abs(step_.inl());
	if ( geom.stop_.inl() == expected_inline )
            isafter = false;
        else
            { mergeIrreg( geom ); return; }
    }

    if ( isafter ) stop_.inl() = geom.stop_.inl();
    else	   start_.inl() = geom.start_.inl();
}


void CBVSInfo::SurvGeom::toIrreg()
{
    if ( !fullyrectandreg_ ) return;

    deepErase( cubedata_ );
    fullyrectandreg_ = false;
    const int nrinls = (stop_.inl()-start_.inl())/ abs(step_.inl()) + 1;
    const int startinl = step_.inl() > 0 ? start_.inl() : stop_.inl();
    for ( int idx=0; idx<nrinls; idx++ )
    {
	int curinl = startinl + idx * step_.inl();
	PosInfo::LineData* newinf = new PosInfo::LineData( curinl );
	newinf->segments_ += PosInfo::LineData::Segment(
				 step_.crl() > 0 ? start_.crl() : stop_.crl(),
				 step_.crl() > 0 ? stop_.crl() : start_.crl(),
				 step_.crl() );
	cubedata_.add( newinf );
    }
}


void CBVSInfo::SurvGeom::mergeIrreg( const CBVSInfo::SurvGeom& g )
{
    const CBVSInfo::SurvGeom* geom = &g;
    if ( geom->fullyrectandreg_ )
    {
	geom = new CBVSInfo::SurvGeom( g );
	const_cast<CBVSInfo::SurvGeom*>(geom)->toIrreg();
    }
    toIrreg();

    for ( int idx=0; idx<geom->cubedata_.size(); idx++ )
    {
	const PosInfo::LineData* gii = geom->cubedata_[idx];
	const int myidx = cubedata_.indexOf( gii->linenr_ );
	PosInfo::LineData* ii = myidx >= 0 ? cubedata_[myidx] : 0;
	if ( !ii )
	    cubedata_.add( new PosInfo::LineData( *gii ) );
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
    if ( fullyrectandreg_ ) return;

    TrcKeySampling hs(false);
    hs.start_ = start_; hs.stop_ = stop_;
    bool firstpos = true;
    for ( int icd=0; icd<cubedata_.size(); icd++ )
    {
	const PosInfo::LineData& ii = *cubedata_[icd];
	for ( int iseg=0; iseg<ii.segments_.size(); iseg++ )
	{
	    const PosInfo::LineData::Segment& seg = ii.segments_[iseg];
	    if ( !seg.start_ && !seg.stop_ )
	    {
#ifdef __debug__
		std::cerr << "CBVSInfo - Empty segment: " << ii.linenr_
			  << " segment " << iseg << std::endl;
#endif
		continue;
	    }

	    if ( firstpos )
	    {
		hs.start_ = hs.stop_ = BinID( ii.linenr_, seg.start_ );
		firstpos = false;
	    }
	    hs.include( BinID(ii.linenr_,seg.start_) );
	    hs.include( BinID(ii.linenr_,seg.stop_) );
	}
    }

    start_ = hs.start_;
    stop_ = hs.stop_;
}


bool CBVSInfo::contributesTo( const TrcKeyZSampling& cs ) const
{
    if ( cs.hsamp_.start_.inl() > geom_.stop_.inl() ||
	 cs.hsamp_.stop_.inl() < geom_.start_.inl() ||
	 cs.hsamp_.start_.crl() > geom_.stop_.crl() ||
	 cs.hsamp_.stop_.crl() < geom_.start_.crl() )
	return false;

    float zend = sd_.start_ + (nrsamples_-1) * sd_.step_;
    if ( sd_.start_ > cs.zsamp_.stop_+1e-7 || zend < cs.zsamp_.start_-1e-7 )
	return false;

    return true;
}
