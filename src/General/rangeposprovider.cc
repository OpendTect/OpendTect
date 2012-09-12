/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/

static const char* rcsID mUnusedVar = "$Id: rangeposprovider.cc,v 1.24 2012-09-12 10:59:03 cvsraman Exp $";

#include "rangeposprovider.h"
#include "survinfo.h"
#include "posinfo2d.h"
#include "iopar.h"
#include "cubesampling.h"
#include "keystrs.h"


Pos::RangeProvider3D::RangeProvider3D()
    : cs_(*new CubeSampling(true))
{
    reset();
}


Pos::RangeProvider3D::RangeProvider3D( const Pos::RangeProvider3D& p )
    : cs_(*new CubeSampling(false))
{
    *this = p;
}


Pos::RangeProvider3D::~RangeProvider3D()
{
    delete &cs_;
}


Pos::RangeProvider3D& Pos::RangeProvider3D::operator =(
					const Pos::RangeProvider3D& p )
{
    if ( &p != this )
    {
	cs_ = p.cs_;
	curbid_ = p.curbid_;
	curz_ = p.curz_;
    }
    return *this;
}


const char* Pos::RangeProvider3D::type() const
{
    return sKey::Range();
}


void Pos::RangeProvider3D::reset()
{
    curbid_ = BinID( cs_.hrg.start.inl, cs_.hrg.start.crl-cs_.hrg.step.crl );
    curz_ = cs_.zrg.stop;
}


bool Pos::RangeProvider3D::toNextPos()
{
    curbid_.crl += cs_.hrg.step.crl;
    if ( curbid_.crl > cs_.hrg.stop.crl )
    {
	curbid_.inl += cs_.hrg.step.inl;
	if ( curbid_.inl > cs_.hrg.stop.inl )
	    return false;
	curbid_.crl = cs_.hrg.start.crl;
    }

    curz_ = cs_.zrg.start;
    return true;
}


#define mZrgEps (1e-6*cs_.zrg.step)

bool Pos::RangeProvider3D::toNextZ()
{
    curz_ += cs_.zrg.step;
    if ( curz_ > cs_.zrg.stop+mZrgEps )
	return toNextPos();
    return true;
}


bool Pos::RangeProvider3D::includes( const BinID& bid, float z ) const
{
    bool issel = cs_.hrg.includes(bid);
    if ( !issel ) return false;
    if ( mIsUdf(z) ) return true;

    return z < cs_.zrg.stop+mZrgEps && z > cs_.zrg.start - mZrgEps;
}


void Pos::RangeProvider3D::usePar( const IOPar& iop )
{
    cs_.usePar( iop );
}


void Pos::RangeProvider3D::fillPar( IOPar& iop ) const
{
    cs_.fillPar( iop );
}


void Pos::RangeProvider3D::getSummary( BufferString& txt ) const
{
    BufferString tmp;
    cs_.hrg.start.fill( tmp.buf() ); txt += tmp; txt += "-";
    cs_.hrg.stop.fill( tmp.buf() ); txt += tmp;
    const int nrsamps = cs_.zrg.nrSteps() + 1;
    if ( nrsamps > 1 )
	{ txt += " ("; txt += nrsamps; txt += " samples)"; }
}


void Pos::RangeProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = cs_.hrg.start; stop = cs_.hrg.stop;
}


void Pos::RangeProvider3D::getZRange( Interval<float>& zrg ) const
{
    assign( zrg, cs_.zrg );
    mDynamicCastGet(StepInterval<float>*,szrg,&zrg)
    if ( szrg )
	szrg->step = cs_.zrg.step;
}


od_int64 Pos::RangeProvider3D::estNrPos() const
{
    return cs_.hrg.totalNr();
}


int Pos::RangeProvider3D::estNrZPerPos() const
{
    return cs_.zrg.nrSteps() + 1;
}


void Pos::RangeProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Range() );
}


Pos::RangeProvider2D::RangeProvider2D()
    : zrg_(SI().zRange(false))
{
    trcrgs_ += StepInterval<int>(1,mUdf(int),1);
    reset();
}


Pos::RangeProvider2D::RangeProvider2D( const Pos::RangeProvider2D& p )
{
    *this = p;
}


Pos::RangeProvider2D& Pos::RangeProvider2D::operator =(
					const Pos::RangeProvider2D& p )
{
    if ( &p == this )
    {
	zrg_ = p.zrg_;
	curtrcidx_ = p.curtrcidx_;
	curlineidx_ =  p.curlineidx_;
	curz_ = p.curz_;
	for ( int idx=0; idx<p.nrLines(); idx++ )
	    addLineID( p.lineID(idx) );
    }
    return *this;
}


const char* Pos::RangeProvider2D::type() const
{
    return sKey::Range();
}


void Pos::RangeProvider2D::reset()
{
    StepInterval<int> trcrg;
    if ( geomids_.size() )
    {
	PosInfo::Line2DData l2d;
	S2DPOS().getGeometry( geomids_[0], l2d );
	trcrg = l2d.trcNrRange();
    }

    curtrcidx_ =  geomids_.size() ? 0 : -1;
    curlineidx_ = 0;
    curz_ = zrg_.stop;
}


bool Pos::RangeProvider2D::toNextPos()
{
    if ( geomids_.isEmpty() )
	return false;

    PosInfo::Line2DData l2d;
    
    while ( true )
    {
	StepInterval<int> trcrg =
	    trcrgs_.validIdx(curlineidx_) ? trcrgs_[curlineidx_]
	    				  : trcrgs_[0];
	S2DPOS().getGeometry( geomids_[curlineidx_], l2d );
	StepInterval<int> linetrcrg = l2d.trcNrRange();
	trcrg.limitTo( linetrcrg );
	curtrcidx_++;
	
	if ( curtrcidx_ <= trcrg.nrSteps() )
	    break;

	curlineidx_++;
	if ( !geomids_.validIdx(curlineidx_) )
	    return false;

	curtrcidx_ = 0;
	continue;
    }
    
    StepInterval<float> zrg = zrg_;
    zrg.limitTo( l2d.zRange() );
    curz_ = zrg.start;
    return true;
}


#undef mZrgEps
#define mZrgEps (1e-6*zrg_.step)

bool Pos::RangeProvider2D::toNextZ()
{
    curz_ += zrg_.step;
    if ( curz_ > zrg_.stop+mZrgEps )
    {
	bool res = toNextPos();
	return res;
    }

    const bool hasgeom = geomids_.validIdx( curlineidx_ );
    PosInfo::Line2DData l2d;
    if ( hasgeom )
	S2DPOS().getGeometry( geomids_[curlineidx_], l2d );
    if ( hasgeom && curz_ > l2d.zRange().stop+mZrgEps )
	return toNextPos();

    return true;
}


int Pos::RangeProvider2D::curNr() const
{
    const bool hasgeom = geomids_.validIdx( curlineidx_ );
    if ( hasgeom )
    {
	StepInterval<int> trcrg =
	    trcrgs_.validIdx(curlineidx_) ? trcrgs_[curlineidx_]
	    				  : trcrgs_[0];
	PosInfo::Line2DData l2d;
	S2DPOS().getGeometry( geomids_[curlineidx_], l2d );
	StepInterval<int> linetrcrg = l2d.trcNrRange();
	trcrg.limitTo( linetrcrg );
	return trcrg.atIndex( curtrcidx_ );
    }

    return curtrcidx_;
}


Coord Pos::RangeProvider2D::curCoord() const
{
    const bool hasgeom = geomids_.validIdx( curlineidx_ );
    PosInfo::Line2DData l2d;
    if ( hasgeom )
    {
	S2DPOS().getGeometry( geomids_[curlineidx_], l2d );
	PosInfo::Line2DPos l2dpos;
	if ( l2d.getPos(curNr(),l2dpos) )
	    return l2dpos.coord_;
    }

    return Coord(0,0);
}


bool Pos::RangeProvider2D::includes( int nr, float z, int lidx ) const
{
    if ( !geomids_.validIdx(lidx) || mIsUdf(z) ) 
	return false;

    const bool inrg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx].includes(nr,false)
					     : trcrgs_[0].includes(nr,false);
    return inrg && (z < zrg_.stop+mZrgEps && z > zrg_.start - mZrgEps);
}


bool Pos::RangeProvider2D::includes( const Coord& c, float z ) const
{ 
    bool found = false;
    int foundlidx = -1;
    for ( int lidx=0; lidx<nrLines(); lidx++ )
    {
	PosInfo::Line2DData l2d;
	S2DPOS().getGeometry( geomids_[lidx], l2d );
	for ( int idx=0; idx<l2d.positions().size(); idx++ )
	{
	    if ( l2d.positions()[idx].coord_ == c )
	    { found = true; foundlidx=lidx; break; }
	}
    }

    if ( !found ) return false;
    if ( mIsUdf(z) ) return true;
    PosInfo::Line2DData l2d;
    S2DPOS().getGeometry( geomids_[foundlidx], l2d );

    return z > zrg_.start-mZrgEps
	&& z < zrg_.stop+mZrgEps
	&& z > l2d.zRange().start-mZrgEps && z < l2d.zRange().stop+mZrgEps;
}


void Pos::RangeProvider2D::getExtent( Interval<int>& rg, int lidx ) const
{
    rg = Interval<int>( mUdf(int), -mUdf(int) );
    if ( geomids_.validIdx(lidx) )
    {
	PosInfo::Line2DData l2d;
	S2DPOS().getGeometry( geomids_[lidx], l2d );
	rg = l2d.trcNrRange();
    }
    else
	rg = trcrgs_[lidx];
}


void Pos::RangeProvider2D::getZRange( Interval<float>& zrg, int lidx ) const
{
    zrg = zrg_;
    if ( geomids_.validIdx(lidx) )
    {
	PosInfo::Line2DData l2d;
	S2DPOS().getGeometry( geomids_[lidx], l2d );
	zrg = l2d.zRange();
    }
}


void Pos::RangeProvider2D::usePar( const IOPar& iop )
{
    if ( !iop.hasKey(sKey::TrcRange()) )
    {
	CubeSampling cs(false); cs.set2DDef();
	if ( cs.usePar(iop) )
	    trcrgs_[0] = cs.hrg.crlRange();
	zrg_ = cs.zrg;
    }
    else
    {
	int lineidx =0;
	StepInterval<int> trcrg;
	while( iop.get(IOPar::compKey(sKey::TrcRange(),lineidx),trcrg) )
	{
	    trcrgs_ += trcrg;
	    lineidx++;
	}

	iop.get( sKey::ZRange(), zrg_);
    }
}


void Pos::RangeProvider2D::fillPar( IOPar& iop ) const
{
    for ( int lidx=0; lidx<trcrgs_.size(); lidx++ )
	iop.set( IOPar::compKey(sKey::TrcRange(),lidx), trcrgs_[lidx] );
    iop.set( sKey::ZRange(), zrg_ );
}


od_int64 Pos::RangeProvider2D::estNrPos() const
{
    od_int64 sz = 0;
    if ( geomids_.size() )
    {
	for ( int idx=0; idx<geomids_.size(); idx++ )
	{
	    PosInfo::Line2DData l2d;
	    S2DPOS().getGeometry( geomids_[idx], l2d );
	    sz += l2d.trcNrRange().nrSteps() + 1;
	}
    }
    else
	sz = trcrgs_[0].nrSteps() + 1;
    return sz;
}


int Pos::RangeProvider2D::estNrZPerPos() const
{ return zrg_.nrSteps()+1; }


void Pos::RangeProvider2D::getSummary( BufferString& txt ) const
{
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	PosInfo::Line2DData l2d;
	if ( !S2DPOS().getGeometry(geomids_[idx],l2d) )
	    continue;
	txt += "Line "; txt += idx; txt += ":";
	StepInterval<int> rg = l2d.trcNrRange();
	StepInterval<float> zrg = zrg_;
	const bool noend = mIsUdf(rg.stop);
	if ( rg.start == 1 && noend )
	    txt += "[all]";
	else
	{
	    txt += rg.start; txt += "-";
	    if ( noend )
		txt += "[last]";
	    else
		txt += rg.stop;
	}
	if ( rg.step != 1 )
	    { txt += " step "; txt += rg.step; }

	txt += " ("; txt += zrg.nrSteps() + 1; txt += " samples)";
	txt += "\n";
    }
}


void Pos::RangeProvider2D::initClass()
{
    Pos::Provider2D::factory().addCreator( create, sKey::Range() );
}
