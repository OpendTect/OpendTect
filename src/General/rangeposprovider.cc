/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/

static const char* rcsID mUnusedVar = "$Id$";

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
    : curlinegeom_(0)
{
    zrgs_ += SI().zRange( false );
    trcrgs_ += StepInterval<int>(1,mUdf(int),1);
    reset();
}


Pos::RangeProvider2D::RangeProvider2D( const Pos::RangeProvider2D& p )
    : curlinegeom_(0)
{
    *this = p;
}


Pos::RangeProvider2D& Pos::RangeProvider2D::operator =(
					const Pos::RangeProvider2D& p )
{
    if ( &p != this )
    {
	zrgs_ = p.zrgs_;
	trcrgs_ = p.trcrgs_;
	curtrcidx_ = p.curtrcidx_;
	curlineidx_ =  p.curlineidx_;
	delete curlinegeom_; curlinegeom_ = 0;
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
    curlineidx_ = 0;
    delete curlinegeom_; curlinegeom_ = 0;
    StepInterval<float> zrg = zrgs_[0];
    if ( !geomids_.isEmpty() )
    {
	PosInfo::Line2DData* linegeom = new PosInfo::Line2DData;
	if ( !S2DPOS().getGeometry(geomids_[0],*linegeom) )
	    delete linegeom;
	else
	{
	    curlinegeom_ = linegeom;
	    zrg.limitTo( curlinegeom_->zRange() );
	}
    }

    curtrcidx_ = -1;
    curz_ = zrg.stop;
}


const PosInfo::Line2DData* Pos::RangeProvider2D::curGeom() const
{
    if ( !curlinegeom_ && geomids_.validIdx(curlineidx_) )
    {
	const PosInfo::GeomID& curgeomid = geomids_[curlineidx_];
	PosInfo::Line2DData* linegeom = new PosInfo::Line2DData;
	if ( !S2DPOS().getGeometry(curgeomid,*linegeom) )
	    delete linegeom;
	else
	    const_cast<Pos::RangeProvider2D*>(this)->curlinegeom_ = linegeom;
    }

    return curlinegeom_;
}


bool Pos::RangeProvider2D::toNextPos()
{
    while ( true )
    {
	StepInterval<int> trcrg = curTrcRange();
	if ( trcrg.isUdf() )
	    return false;

	curtrcidx_++;
	if ( curtrcidx_ <= trcrg.nrSteps() )
	    break;

	curlineidx_++;
	delete curlinegeom_; curlinegeom_ = 0;
	if ( !geomids_.validIdx(curlineidx_) )
	    return false;

	curtrcidx_ = -1;
    }
    
    StepInterval<float> zrg = curZRange();
    curz_ = zrg.start;
    return true;
}


#undef mZrgEps
#define mZrgEps (1e-6*zrg.step)

bool Pos::RangeProvider2D::toNextZ()
{
    StepInterval<float> zrg = curZRange();
    curz_ += zrg.step;
    if ( curz_ > zrg.stop+mZrgEps )
	return toNextPos();

    return true;
}


int Pos::RangeProvider2D::curNr() const
{
    StepInterval<int> trcrg = curTrcRange();
    return trcrg.atIndex( curtrcidx_ );
}


Coord Pos::RangeProvider2D::curCoord() const
{
    const PosInfo::Line2DData* curgeom = curGeom();
    if ( curgeom )
    {
	PosInfo::Line2DPos l2dpos;
	if ( curgeom->getPos(curNr(),l2dpos) )
	    return l2dpos.coord_;
    }

    return Coord::udf();
}


StepInterval<int> Pos::RangeProvider2D::curTrcRange() const
{
    StepInterval<int> trcrg = trcrgs_.validIdx(curlineidx_) ?
					trcrgs_[curlineidx_] : trcrgs_[0];
    const PosInfo::Line2DData* curgeom = curGeom();
    if ( curgeom )
	trcrg.limitTo( curGeom()->trcNrRange() );

    return trcrg;
}


StepInterval<float> Pos::RangeProvider2D::curZRange() const
{
    StepInterval<float> zrg = zrgs_.validIdx(curlineidx_) ? zrgs_[curlineidx_]
							  : zrgs_[0];
    const PosInfo::Line2DData* curgeom = curGeom();
    if ( curgeom )
	zrg.limitTo( curGeom()->zRange() );

    return zrg;
}


bool Pos::RangeProvider2D::includes( int nr, float z, int lidx ) const
{
    if ( lidx == curlineidx_ || !geomids_.validIdx(lidx) )
	return curTrcRange().includes(nr,false)
	    && curZRange().includes(z,false);

    PosInfo::Line2DData l2d;
    if ( !S2DPOS().getGeometry(geomids_[lidx],l2d) )
	return false;

    StepInterval<int> trcrg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx]
						     : trcrgs_[0];
    trcrg.limitTo( l2d.trcNrRange() );
    StepInterval<float> zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx] : zrgs_[0];
    zrg.limitTo( l2d.zRange() );
    return trcrg.includes(nr,false) && zrg.includes(z,false);
}


bool Pos::RangeProvider2D::includes( const Coord& c, float z ) const
{ 
    const PosInfo::Line2DData* curgeom = curGeom();
    if ( curgeom )
    {
	const TypeSet<PosInfo::Line2DPos>& pos = curgeom->positions();
	for ( int idx=0; idx<pos.size(); idx++ )
	    if ( pos[idx].coord_ == c )
		return curTrcRange().includes(pos[idx].nr_,false)
	    		&& curZRange().includes(z,false);
    }

    for ( int lidx=0; lidx<nrLines(); lidx++ )
    {
	PosInfo::Line2DData l2d;
	if ( !S2DPOS().getGeometry(geomids_[lidx],l2d) )
	    continue;

	const TypeSet<PosInfo::Line2DPos>& pos = curgeom->positions();
	for ( int idx=0; idx<pos.size(); idx++ )
	{
	    if ( pos[idx].coord_ == c )
	    {
		StepInterval<int> trcrg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx]
		    						 : trcrgs_[0];
		StepInterval<float> zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx]
		    					       : zrgs_[0];
		zrg.limitTo( l2d.zRange() );
		return trcrg.includes(pos[idx].nr_,false)
		    	&& zrg.includes(z,false);
	    }
	}
    }

    return false;
}


void Pos::RangeProvider2D::getExtent( Interval<int>& rg, int lidx ) const
{
    if ( lidx == curlineidx_ || !geomids_.validIdx(lidx) )
    { rg = curTrcRange(); return; }

    rg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx] : trcrgs_[0];
    PosInfo::Line2DData l2d;
    if ( !S2DPOS().getGeometry(geomids_[lidx],l2d) )
	return;

    rg.limitTo( l2d.trcNrRange() );
}


void Pos::RangeProvider2D::getZRange( Interval<float>& zrg, int lidx ) const
{
    if ( lidx == curlineidx_ || !geomids_.validIdx(lidx) )
    { zrg = curZRange(); return; }

    zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx] : zrgs_[0];
    PosInfo::Line2DData l2d;
    if ( !S2DPOS().getGeometry(geomids_[lidx],l2d) )
	return;

    zrg.limitTo( l2d.zRange() );
}


void Pos::RangeProvider2D::usePar( const IOPar& iop )
{
    PtrMan<IOPar> subpartrcrg = iop.subselect( sKey::TrcRange() );
    if ( !subpartrcrg )
    {
	CubeSampling cs(false); cs.set2DDef();
	if ( cs.usePar(iop) )
	{
	    trcrgs_[0] = cs.hrg.crlRange();
	    zrgs_[0] = cs.zrg;
	}

	return;
    }

    int idx = 0;
    trcrgs_.erase();
    StepInterval<int> trcrg;
    while ( subpartrcrg->get(toString(idx++),trcrg) )
	trcrgs_ += trcrg;

    if ( trcrgs_.isEmpty() )
	trcrgs_ += StepInterval<int>( 1, mUdf(int), 1 );

    PtrMan<IOPar> subpargeom = iop.subselect( sKey::GeomID() );
    if ( !subpargeom )
	subpargeom = iop.subselect( sKey::ID() );
    if ( subpargeom )
    {
	idx = 0;
	geomids_.erase();
	BufferString str;
	PosInfo::GeomID geomid;
	while ( subpargeom->get(toString(idx++),str) && geomid.fromString(str) )
	    addLineID( geomid );
    }

    PtrMan<IOPar> subparzrg = iop.subselect( sKey::ZRange() );
    if ( subparzrg )
    {
	idx = 0;
	zrgs_.erase();
	StepInterval<float> zrg;
	while ( subparzrg->get(toString(idx++),zrg) )
	    zrgs_ += zrg;

	if ( zrgs_.isEmpty() )
	    zrgs_ += SI().zRange( false );
    }
}


void Pos::RangeProvider2D::fillPar( IOPar& iop ) const
{
    for ( int lidx=0; lidx<geomids_.size(); lidx++ )
	iop.set( IOPar::compKey(sKey::GeomID(),lidx),
		 geomids_[lidx].toString() );
    for ( int lidx=0; lidx<trcrgs_.size(); lidx++ )
	iop.set( IOPar::compKey(sKey::TrcRange(),lidx), trcrgs_[lidx] );
    for ( int lidx=0; lidx<zrgs_.size(); lidx++ )
	iop.set( IOPar::compKey(sKey::ZRange(),lidx), zrgs_[lidx] );
}


od_int64 Pos::RangeProvider2D::estNrPos() const
{
    od_int64 sz = 0;
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	PosInfo::Line2DData l2d;
	if ( !S2DPOS().getGeometry(geomids_[idx],l2d) )
	    continue;

	sz += l2d.positions().size();
    }

    if ( !sz && !trcrgs_[0].isUdf() )
	sz = trcrgs_[0].nrSteps() + 1;

    return sz;
}


int Pos::RangeProvider2D::estNrZPerPos() const
{ return zrgs_[0].nrSteps()+1; }


void Pos::RangeProvider2D::getSummary( BufferString& txt ) const
{
    for ( int idx=0; idx<trcrgs_.size(); idx++ )
    {
	txt += "Line "; txt += idx; txt += ":";
	const StepInterval<int>& rg = trcrgs_[0];
	const StepInterval<float>& zrg = zrgs_.validIdx(idx) ? zrgs_[idx]
	    						     : zrgs_[0];
	if ( rg.isUdf() )
	    txt += "[all]";
	else
	{
	    txt += rg.start; txt += "-";
	    txt += rg.stop;
	    if ( rg.step != 1 )
	    { txt += " step "; txt += rg.step; }
	}

	txt += " ("; txt += zrg.nrSteps() + 1; txt += " samples)";
	txt += "\n";
    }
}


void Pos::RangeProvider2D::initClass()
{ Pos::Provider2D::factory().addCreator( create, sKey::Range() ); }
