/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/


#include "posinfo2d.h"
#include "posinfo.h"
#include "math2.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "linesubsel.h"
#include "od_iostream.h"

#define mUseL2DDataType(typ) mUseType( PosInfo::Line2DData, typ )
mUseL2DDataType(trcnr_type);
mUseL2DDataType(IdxSet);
mUseL2DDataType(TrcNrSet);
mUseL2DDataType(PosSet);
mUseL2DDataType(idx_type);
mUseL2DDataType(size_type);
mUseL2DDataType(dist_type);
mUseL2DDataType(z_type);
mUseL2DDataType(z_steprg_type);


PosInfo::Line2DData::Line2DData( const char* lnm )
    : lnm_(lnm)
    , zrg_(SI().zRange())
{
}


// Returns the index, or the index just before if not found

idx_type PosInfo::Line2DData::gtIndex( trcnr_type nr, bool& found ) const
{
    const size_type sz = posns_.size();
    if ( sz==0 )
	{ found = false; return -1; }

    idx_type i0 = 0, i1 = sz - 1;
    trcnr_type nr0 = posns_[i0].nr_; trcnr_type nr1 = posns_[i1].nr_;
    if ( nr < nr0 )
	{ found = false; return -1; }
    if ( nr > nr1 )
	{ found = false; return sz-1; }

    found = true;
    if ( nr == nr0 || nr == nr1 )
	return nr == nr0 ? 0 : sz-1;

    while ( i1 - i0 > 1 )
    {
	idx_type newi = (i0 + i1) / 2;
	trcnr_type newnr = posns_[newi].nr_;
	if ( newnr == nr )
	    return newi;
	if ( newnr > nr )	{ i1 = newi; nr1 = newnr; }
	else			{ i0 = newi; nr0 = newnr; }
    }

    found = false; return i0;
}


idx_type PosInfo::Line2DData::getClosestBPSegment( const Coord& pt ) const
{
    if ( bendpoints_.isEmpty() )
	return -1;

    dist_type mindiff = mUdf(dist_type);
    idx_type ret = -1;
    for ( idx_type idx=1; idx<bendpoints_.size(); idx++ )
    {
	const Coord start = posns_[bendpoints_[idx-1]].coord_;
	const Coord stop = posns_[bendpoints_[idx]].coord_;
	const dist_type seglength = start.distTo<dist_type>( stop );
	const dist_type distsum = start.distTo<dist_type>( pt ) +
			       pt.distTo<dist_type>( stop );
	const dist_type absdiff = Math::Abs( seglength - distsum );
	if ( absdiff < mindiff )
	{
	    mindiff = absdiff;
	    ret = idx - 1;
	}
    }

    return ret;
}


idx_type PosInfo::Line2DData::gtIndex( const Coord& crd,
				       dist_type* sqdist ) const
{
    if ( posns_.isEmpty() )
	return -1;

#   define mSqDist(idx) posns_[idx].coord_.sqDistTo( crd )

    const idx_type bpidx = getClosestBPSegment( crd );
    if ( bpidx < 0 )
	return -1;

    idx_type i0 = bendpoints_[bpidx], i1 = bendpoints_[bpidx+1];
    dist_type sqd0 = mSqDist( i0 );
    dist_type sqd1 = mSqDist( i1 );

    // Damped bisection because lines can have varying distance steps
    while ( i1 - i0 > 2 )
    {
	if ( sqd0 < sqd1 )
	    { i1 = (2*i1 + i0) / 3; sqd1 = mSqDist( i1 ); }
	else
	    { i0 = (2*i0 + i1) / 3; sqd0 = mSqDist( i0 ); }
    }
    if ( i1 - i0 == 2 )
    {
	if ( sqd0 < sqd1 )
	    { i1--; sqd1 = mSqDist( i1 ); }
	else
	    { i0++; sqd0 = mSqDist( i0 ); }
    }
    if ( sqdist )
	*sqdist = sqd0 < sqd1 ? sqd0 : sqd1;

    return sqd0 < sqd1 ? i0 : i1;
}


void PosInfo::Line2DData::setLineName( const char* lnm )
{
    lnm_ = lnm;
    geomid_.setInvalid();
}


Pos::GeomID PosInfo::Line2DData::geomID() const
{
    if ( !geomid_.isValid() && !lnm_.isEmpty() )
	geomid_ = SurvGeom::getGeomID( lnm_ );
    return geomid_;
}


void PosInfo::Line2DData::setGeomID( GeomID gid ) const
{
    geomid_ = gid;
    if ( lnm_.isEmpty() && geomid_.isValid() )
	mSelf().lnm_ = geomid_.name();
}


Bin2D PosInfo::Line2DData::bin2D( idx_type idx ) const
{
    return Bin2D( geomid_, posns_.validIdx(idx) ? posns_.get(idx).nr_ : -1 );
}


trcnr_type PosInfo::Line2DData::trcNr( idx_type idx ) const
{
    return posns_.validIdx(idx) ? posns_.get(idx).nr_ : -1;
}


Coord PosInfo::Line2DData::coord( idx_type idx ) const
{
    return posns_.validIdx(idx) ? posns_.get(idx).coord_ : -1;
}


void PosInfo::Line2DData::add( const Line2DPos& pos )
{
    const size_type sz = posns_.size();
    if ( sz == 0 || pos.nr_ > posns_[sz-1].nr_ )
	{ posns_ += pos; return; }

    bool found; idx_type idx = gtIndex( pos.nr_, found );
    if ( !found )
	posns_.insert( idx+1, pos );
}


void PosInfo::Line2DData::removeByIdx( idx_type idx )
{
    if ( posns_.validIdx(idx) )
	posns_.removeSingle( idx );
}


void PosInfo::Line2DData::remove( trcnr_type nr )
{
    bool found; int idx = gtIndex( nr, found );
    if ( found )
	posns_.removeSingle( idx );
}


void PosInfo::Line2DData::limitTo( trcnr_type start, trcnr_type stop )
{
    if ( start > stop )
	std::swap( start, stop );
    for ( idx_type idx=posns_.size()-1; idx!=-1; idx-- )
    {
	const auto&pos = posns_[idx];
	if ( pos.nr_ < start || pos.nr_ > stop )
	    posns_.removeSingle( idx );
    }
}


idx_type PosInfo::Line2DData::indexOf( trcnr_type nr ) const
{
    bool found; idx_type idx = gtIndex( nr, found );
    return found ? idx : -1;
}


idx_type PosInfo::Line2DData::nearestIdx( const Coord& pos,
				     const trcnr_rg_type& nrrg ) const
{
    const idx_type posidx = gtIndex( pos );
    if ( !posns_.validIdx(posidx) )
	return -1;

    if ( nrrg.includes(posns_[posidx].nr_,true) )
	return posidx;

    const idx_type posstartidx = posns_.indexOf( nrrg.start );
    const idx_type posstopidx = posns_.indexOf( nrrg.stop );
    const dist_type sqd0 = pos.sqDistTo(posns_[posstartidx].coord_);
    const dist_type sqd1 = pos.sqDistTo(posns_[posstopidx].coord_);
    return sqd0 < sqd1 ? posstartidx : posstopidx;
}


bool PosInfo::Line2DData::getPos( const Coord& crd,
				  Line2DPos& pos, dist_type* dist ) const
{
    dist_type sqdist;
    const idx_type idx = gtIndex( crd, &sqdist );
    if ( !posns_.validIdx(idx) )
	return false;

    pos = posns_[idx];
    if ( dist )
	*dist = Math::Sqrt( sqdist );
    return true;
}


bool PosInfo::Line2DData::getPos( const Coord& crd,
				  Line2DPos& pos, dist_type maxdist ) const
{
    dist_type dist;
    return getPos(crd,pos,&dist) && dist < maxdist;
}


bool PosInfo::Line2DData::getPos( trcnr_type nr, Line2DPos& pos ) const
{
    bool found; idx_type idx = gtIndex( nr, found );
    if ( !found )
	return false;
    pos = posns_[idx];
    return true;
}


void PosInfo::Line2DData::dump( od_ostream& strm, bool pretty ) const
{
    if ( !pretty )
	strm << zrg_.start << '\t' << zrg_.stop << '\t' << zrg_.step << '\n';
    else
    {
	strm << lnm_ << '\n';
	const auto fac = SI().zDomain().userFactor();
	strm << "Z range (" << toString( SI().zUnitString() )
	     << "):\t" << fac*zrg_.start
	     << '\t' << fac*zrg_.stop << "\t" << fac*zrg_.step;
	strm << "\n\nTrcNr\tX-coord\tY-coord" << od_newline;
    }

    for ( idx_type idx=0; idx<posns_.size(); idx++ )
    {
	const PosInfo::Line2DPos& pos = posns_[idx];
	strm << pos.nr_ << '\t'
	     << pos.coord_.x_ << '\t'
	     << pos.coord_.y_ << '\n';
    }
    strm.flush();
}


bool PosInfo::Line2DData::read( od_istream& strm, bool asc )
{
    size_type linesz = -1;
    if ( asc )
	strm >> zrg_.start >> zrg_.stop >> zrg_.step >> linesz;
    else
	strm.getBin( zrg_.start ).getBin( zrg_.stop ).getBin( zrg_.step )
	    .getBin( linesz );


    if ( !strm.isOK() || linesz < 0 )
	return false;

    posns_.erase();
    bendpoints_.erase();
    for ( idx_type idx=0; idx<linesz; idx++ )
    {
	trcnr_type trcnr = -1;
	if ( asc )
	    strm >> trcnr;
	else
	    strm.getBin( trcnr );
	if ( trcnr<0 || !strm.isOK() )
	    return false;

	PosInfo::Line2DPos pos( trcnr );
	if ( asc )
	    strm >> pos.coord_.x_ >> pos.coord_.y_;
	else
	    strm.getBin( pos.coord_.x_ ).getBin( pos.coord_.y_ );
	posns_ += pos;
    }

    return true;
}


bool PosInfo::Line2DData::write( od_ostream& strm, bool asc,
				 bool withnls ) const
{
    const size_type linesz = posns_.size();
    if ( !asc )
	strm.addBin( zrg_.start ).addBin( zrg_.stop ).addBin( zrg_.step )
	    .addBin( linesz );
    else
    {
	strm << zrg_.start << ' ' << zrg_.stop << ' ' << zrg_.step
	     << ' ' << linesz;
	if ( withnls && linesz ) strm << od_newline;
    }

    for ( idx_type idx=0; idx<linesz; idx++ )
    {
	const PosInfo::Line2DPos& pos = posns_[idx];
	if ( !asc )
	    strm.addBin(pos.nr_).addBin(pos.coord_.x_).addBin(pos.coord_.y_);
	else
	{
	    BufferString str; str.set( pos.coord_.x_ );
	    strm << '\t' << pos.nr_ << '\t' << str;
	    str.set( pos.coord_.y_ );
	    strm << '\t' << str;
	    if ( withnls && idx < linesz-1 ) strm << od_newline;
	}
    }

    return strm.isOK();
}


void PosInfo::Line2DData::getBendPositions(
			TypeSet<PosInfo::Line2DPos>& bendpos) const
{
    bendpos.erase();
    for ( idx_type idx=0; idx<bendpoints_.size(); idx++ )
	bendpos.add( posns_[bendpoints_[idx]] );
}


const TypeSet<idx_type>& PosInfo::Line2DData::getBendPoints() const
{
    return bendpoints_;
}


void PosInfo::Line2DData::setBendPoints( const IdxSet& bps )
{
    bendpoints_ = bps;
}


PosInfo::Line2DData::trcnr_steprg_type PosInfo::Line2DData::trcNrRange() const
{
    const size_type sz = posns_.size();
    trcnr_steprg_type res( -1, -1, 1 );
    if ( sz < 1 )
	return res;
    res.start = posns_[0].nr_;
    res.stop = posns_[sz-1].nr_;
    if ( sz == 1 )
	return res;

    res.step = 0;
    for ( idx_type idx=1; idx<sz; idx++ )
    {
	const trcnr_type diff = posns_[idx].nr_ - posns_[idx-1].nr_;
	if ( diff == 0 )
	    continue;

	if ( res.step == 0 )
	    res.step = diff;
	else if ( res.step < 0 && diff > res.step )
	    res.step = diff;
	else if ( res.step > 0 && diff < res.step )
	    res.step = diff;

	if ( res.step == 1 || res.step == -1 )
	    break;
    }

    if ( res.step == 0 )
	res.step = 1;

    if ( res.start > res.stop )
	std::swap( res.start, res.stop );
    if ( res.step < 0 )
	res.step = -res.step;
    return res;
}


Coord PosInfo::Line2DData::getNormal( trcnr_type trcnr ) const
{
    bool found; const idx_type posidx = gtIndex( trcnr, found );
    if ( !found )
	return Coord::udf();

    Coord pos = posns_[posidx].coord_;
    Coord v1;
    if ( posidx+1<posns_.size() )
	v1 = posns_[posidx+1].coord_ - pos;
    else if ( posidx-1>=0 )
	v1 = pos - posns_[posidx-1].coord_;

    if ( v1.x_ == 0 )
	return Coord( 1, 0 );
    else if ( v1.y_ == 0 )
	return Coord( 0, 1 );
    else
    {
	const dist_type length = Math::Sqrt( v1.x_*v1.x_ + v1.y_*v1.y_ );
	return Coord( -v1.y_/length, v1.x_/length );
    }
}


dist_type PosInfo::Line2DData::distBetween( trcnr_type starttrcnr,
					    trcnr_type stoptrcnr ) const
{
    dist_type ret = mUdf(dist_type);
    if ( stoptrcnr < starttrcnr )
	return ret;
    bool found; const idx_type startidx = gtIndex( starttrcnr, found );
    if ( !found )
	return ret;
    const idx_type stopidx = gtIndex( stoptrcnr, found );
    if ( !found )
	return ret;

    ret = 0;
    for ( idx_type idx=startidx; idx<stopidx; idx++ )
	ret += posns_[idx+1].coord_.distTo<dist_type>( posns_[idx].coord_ );
    return ret;
}


void PosInfo::Line2DData::getTrcDistStats( dist_type& max,
					   dist_type& median ) const
{
    const size_type nrpositions = posns_.size();
    max = median = 0;
    if ( nrpositions < 2 )
	return;

    dist_type maxsqdist = 0;
    TypeSet<dist_type> distsqs;
    for ( idx_type idx=1; idx<posns_.size(); idx++ )
    {
	const dist_type distsq = posns_[idx].coord_
				.sqDistTo( posns_[idx-1].coord_ );
	if ( mIsUdf(distsq) || mIsZero(distsq, 1e-3) )
	    continue;
	if ( distsq > maxsqdist )
	    maxsqdist = distsq;
	distsqs += distsq;
    }
    sort( distsqs );

    median = Math::Sqrt( distsqs[distsqs.size()/2] );
    max = Math::Sqrt( maxsqdist );
}


bool PosInfo::Line2DData::coincidesWith( const PosInfo::Line2DData& oth ) const
{
    const TypeSet<Line2DPos>& mypos = positions();
    const TypeSet<Line2DPos>& othpos = oth.positions();
    if ( mypos.isEmpty() || othpos.isEmpty() )
	return false;

    const trcnr_type startnr = mMAX( mypos.first().nr_, othpos.first().nr_ );
    const trcnr_type stopnr = mMIN( mypos.last().nr_, othpos.last().nr_ );
    bool found = false;
    const idx_type mystartidx = gtIndex( startnr, found );
    const idx_type mystopidx = gtIndex( stopnr, found );
    const idx_type othstartidx = oth.gtIndex( startnr, found );
    const idx_type othstopidx = oth.gtIndex( stopnr, found );
    if ( mystartidx < 0 || mystopidx < 0 || othstartidx < 0 || othstopidx < 0 )
	return false;

    idx_type myidx = mystartidx, othidx = othstartidx;
    bool foundcommon = false;
    while ( myidx <= mystopidx && othidx <= othstopidx )
    {
	const trcnr_type trcnr = mypos[myidx].nr_;
	if ( trcnr == othpos[othidx].nr_ )
	{
	    foundcommon = true;
	    if ( !mIsEqual(mypos[myidx].coord_.x_,
			   othpos[othidx].coord_.x_,1.0) ||
		 !mIsEqual(mypos[myidx].coord_.y_,
			   othpos[othidx].coord_.y_,1.0) )
	    {
		return false;
	    }

	    myidx++; othidx++;
	}
	else if ( trcnr < othpos[othidx].nr_ )
	    myidx++;
	else
	    othidx++;
    }

    return foundcommon;
}


void PosInfo::Line2DData::getSegments( LineData& ld ) const
{
    ld.segments_.setEmpty();
    const size_type nrposns = posns_.size();
    if ( nrposns < 1 )
	return;

    bool havestep = false;
    trcnr_type prevnr = posns_[0].nr_;
    LineData::Segment curseg( prevnr, prevnr, 1 );
    if ( nrposns < 2 )
	{ ld.segments_ += curseg; return; }

    for ( idx_type ipos=1; ipos<nrposns; ipos++ )
    {
	const trcnr_type curnr = posns_[ipos].nr_;
	if ( curnr == prevnr )
	    continue;

	const trcnr_type curstep = curnr - prevnr;
	if ( !havestep )
	{
	    curseg.step = curstep;
	    havestep = true;
	}
	else if ( curstep != curseg.step )
	{
	    ld.segments_ += curseg;
	    curseg.start = curnr;
	    havestep = false;
	}

	curseg.stop = curnr;
	prevnr = curnr;

	if ( ipos == nrposns-1 )
	    ld.segments_ += curseg;
    }
}


LineSubSel* PosInfo::Line2DData::getSubSel() const
{
    const auto geomid = geomID();
    LineSubSel* lss;
    if ( !geomid.isValid() )
	lss = new LineSubSel( trcNrRange() );
    else
    {
	lss = new LineSubSel( geomid );
	lss->lineHorSubSel().setOutputPosRange( trcNrRange() );
    }
    lss->zSubSel().setOutputZRange( zRange() );
    return lss;
}


void PosInfo::Line2DDataSet::getSubSel( LineSubSelSet& lsss ) const
{
    lsss.setEmpty();
    for ( auto l2dd : *this )
	lsss.add( l2dd->getSubSel() );
}


od_int64 PosInfo::Line2DDataSet::totalNrPositions() const
{
    od_int64 ret = 0;
    for ( auto l2dd : *this )
	ret += l2dd->size();
    return ret;
}


idx_type PosInfo::Line2DDataSet::lineIndexOf( GeomID gid ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( get(idx)->geomID() == gid )
	    return idx;
    return -1;
}


PosInfo::Line2DData* PosInfo::Line2DDataSet::doFind( GeomID gid ) const
{
    const auto idx = lineIndexOf( gid );
    return idx<0 ? 0 : mNonConst( get(idx) );
}
