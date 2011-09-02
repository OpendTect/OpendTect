/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/

static const char* rcsID = "$Id: rangeposprovider.cc,v 1.16 2011-09-02 09:12:13 cvskris Exp $";

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
    return sKey::Range;
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
    Pos::Provider3D::factory().addCreator( create, sKey::Range );
}


Pos::RangeProvider2D::RangeProvider2D()
    : rg_(1,mUdf(int),1)
    , zrg_(SI().zRange(false))
{
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
	rg_ = p.rg_;
	zrg_ = p.zrg_;
	curidx_ = p.curidx_;
	curz_ = p.curz_;
	setLineData( p.ld_ ? new PosInfo::Line2DData(*p.ld_) : 0);
    }
    return *this;
}


const char* Pos::RangeProvider2D::type() const
{
    return sKey::Range;
}


void Pos::RangeProvider2D::reset()
{
    curidx_ = ld_ ? -1 : rg_.start - rg_.step;
    curz_ = zrg_.stop;
}


bool Pos::RangeProvider2D::toNextPos()
{
    while ( true )
    {
	if ( !ld_ )
	    curidx_ += rg_.step;
	else
	{
	    curidx_++;
	    if ( curidx_ >= ld_->positions().size() )
		return false;
	}

	const int curnr = curNr();
	if ( curnr < rg_.start )
	    continue;
	if ( curnr > rg_.stop )
	    return false;
	if ( (curnr-rg_.start) % rg_.step == 0 )
	    break;
    }

    curz_ = zrg_.start;
    if ( ld_ )
    {
	if ( curz_ < ld_->zRange().start ) curz_ = ld_->zRange().start;
	if ( curz_ > ld_->zRange().stop ) return false;
    }

    return true;
}


#undef mZrgEps
#define mZrgEps (1e-6*zrg_.step)

bool Pos::RangeProvider2D::toNextZ()
{
    curz_ += zrg_.step;
    if ( curz_ > zrg_.stop+mZrgEps )
	return toNextPos();

    if ( ld_ && curz_ > ld_->zRange().stop+mZrgEps )
	return toNextPos();

    return true;
}


int Pos::RangeProvider2D::curNr() const
{
    return ld_ ? ld_->positions()[curidx_].nr_ : curidx_;
}


Coord Pos::RangeProvider2D::curCoord() const
{
    return ld_ ? ld_->positions()[curidx_].coord_ : Coord(0,0);
}


bool Pos::RangeProvider2D::includes( int nr, float z ) const
{
    bool issel = rg_.includes( nr, true );
    if ( !issel ) return false;
    if ( mIsUdf(z) ) return true;

    return z < zrg_.stop+mZrgEps && z > zrg_.start - mZrgEps;
}


bool Pos::RangeProvider2D::includes( const Coord& c, float z ) const
{
    if ( !ld_ ) return false;

    bool found = false;
    for ( int idx=0; idx<ld_->positions().size(); idx++ )
    {
	if ( ld_->positions()[idx].coord_ == c )
	    { found = true; break; }
    }
    if ( !found ) return false;
    if ( mIsUdf(z) ) return true;

    return z > zrg_.start - mZrgEps && z < zrg_.stop+mZrgEps
	&& z > ld_->zRange().start-mZrgEps && z < ld_->zRange().stop+mZrgEps;
}


void Pos::RangeProvider2D::usePar( const IOPar& iop )
{
    CubeSampling cs(false); cs.set2DDef();
    if ( cs.usePar(iop) )
	rg_ = cs.hrg.crlRange();
    iop.get( sKey::ZRange, zrg_ );
}


void Pos::RangeProvider2D::fillPar( IOPar& iop ) const
{
    CubeSampling cs(false); cs.set2DDef();
    cs.hrg.start.crl = rg_.start; cs.hrg.stop.crl = rg_.stop;
    cs.hrg.step.crl = rg_.step; cs.hrg.fillPar( iop );
    iop.set( sKey::ZRange, zrg_ );
}


void Pos::RangeProvider2D::getSummary( BufferString& txt ) const
{
    const bool noend = mIsUdf(rg_.stop);
    if ( rg_.start == 1 && noend )
	txt += "[all]";
    else
    {
	txt += rg_.start; txt += "-";
	if ( noend )
	    txt += "[last]";
	else
	    txt += rg_.stop;
    }
    if ( rg_.step != 1 )
	{ txt += " step "; txt += rg_.step; }

    txt += " ("; txt += zrg_.nrSteps() + 1; txt += " samples)";
}


void Pos::RangeProvider2D::initClass()
{
    Pos::Provider2D::factory().addCreator( create, sKey::Range );
}
