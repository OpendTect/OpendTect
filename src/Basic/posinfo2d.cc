/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "posinfo2d.h"
#include "math2.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


PosInfo::Line2DData::Line2DData( const char* lnm )
    : xrg_(Interval<double>::udf())
    , yrg_(Interval<double>::udf())
    , zrg_(SI().sampling(false).zsamp_)
    , lnm_(lnm)
{
}


PosInfo::Line2DData::Line2DData( const Line2DData& l2d )
{
    *this = l2d;
}


PosInfo::Line2DData::~Line2DData()
{}


PosInfo::Line2DData& PosInfo::Line2DData::operator=( const Line2DData& oth )
{
    if ( this == &oth )
	return *this;

    xrg_ = oth.xrg_;
    yrg_ = oth.yrg_;
    zrg_ = oth.zrg_;
    lnm_ = oth.lnm_;
    posns_ = oth.posns_;
    bendpoints_ = oth.bendpoints_;

    return *this;
}


// Returns the index, or the index just before if not found
int PosInfo::Line2DData::gtIndex( int nr, bool& found ) const
{
    const int sz = posns_.size();
    if ( sz==0 )
	{ found = false; return -1; }

    int i0 = 0, i1 = sz - 1;
    int nr0 = posns_[i0].nr_; int nr1 = posns_[i1].nr_;
    if ( nr < nr0 )
	{ found = false; return -1; }
    if ( nr > nr1 )
	{ found = false; return sz-1; }

    found = true;
    if ( nr == nr0 || nr == nr1 )
	return nr == nr0 ? 0 : sz-1;

    while ( i1 - i0 > 1 )
    {
	int newi = (i0 + i1) / 2;
	int newnr = posns_[newi].nr_;
	if ( newnr == nr )
	    return newi;
	if ( newnr > nr )	{ i1 = newi; nr1 = newnr; }
	else			{ i0 = newi; nr0 = newnr; }
    }

    found = false; return i0;
}


// Returns the index of the first bend point defining the segment closest to pt
int PosInfo::Line2DData::getSegmentIndexClosestToPoint( const Coord& pt ) const
{
    if ( bendpoints_.isEmpty() )
	return -1;

    double mindiff = mUdf(double);
    int ret = -1;
    for ( int idx=1; idx<bendpoints_.size(); idx++ )
    {
	const Coord& start = posns_[bendpoints_[idx-1]].coord_;
	const Coord& stop = posns_[bendpoints_[idx]].coord_;
	const double seglength = start.distTo( stop );
	const double distsum = start.distTo( pt ) +
			       pt.distTo( stop );
	const double absdiff = Math::Abs( seglength - distsum );
	if ( absdiff < mindiff )
	{
	    mindiff = absdiff;
	    ret = idx - 1;
	}
    }

    return ret;
}


int PosInfo::Line2DData::gtIndex( const Coord& coord, double* sqdist ) const
{
    if ( posns_.isEmpty() )
	return -1;

#   define mSqDist(idx) posns_[idx].coord_.sqDistTo( coord )

    const int segidx = getSegmentIndexClosestToPoint( coord );
    if ( segidx < 0 )
	return -1;

    int i0 = bendpoints_[segidx], i1 = bendpoints_[segidx+1];
    double sqd0 = mSqDist( i0 );
    double sqd1 = mSqDist( i1 );

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


void PosInfo::Line2DData::add( const Line2DPos& pos )
{
    const int sz = posns_.size();
    if ( sz == 0 || pos.nr_ > posns_[sz-1].nr_ )
	{ posns_ += pos; return; }

    bool found; int idx = gtIndex( pos.nr_, found );
    if ( found ) return;
    posns_.insert( idx+1, pos );
}


void PosInfo::Line2DData::remove( int nr )
{
    bool found; int idx = gtIndex( nr, found );
    if ( !found ) return;
    posns_.removeSingle( idx );
}


void PosInfo::Line2DData::limitTo( Interval<int> trcrg )
{
    trcrg.sort();
    for ( int idx=0; idx<posns_.size(); idx++ )
	if ( posns_[idx].nr_ < trcrg.start || posns_[idx].nr_ > trcrg.stop )
	    posns_.removeSingle( idx-- );
}


int PosInfo::Line2DData::indexOf( int nr ) const
{
    bool found; int idx = gtIndex( nr, found );
    return found ? idx : -1;
}


int PosInfo::Line2DData::nearestIdx( const Coord& pos,
				     const Interval<int>& nrrg ) const
{
    const int posidx = gtIndex( pos );
    if ( !posns_.validIdx(posidx) )
	return -1;

    if ( nrrg.includes(posns_[posidx].nr_,true) )
	return posidx;

    const int posstartidx = posns_.indexOf( nrrg.start );
    const int posstopidx = posns_.indexOf( nrrg.stop );
    double sqd0 = pos.sqDistTo(posns_[posstartidx].coord_);
    double sqd1 = pos.sqDistTo(posns_[posstopidx].coord_);
    return sqd0 < sqd1 ? posstartidx : posstopidx;
}


bool PosInfo::Line2DData::getPos( const Coord& coord,
				  PosInfo::Line2DPos& pos, float* dist ) const
{
    double sqdist;
    const int idx = gtIndex( coord, &sqdist );
    if ( !posns_.validIdx(idx) )
	return false;

    pos = posns_[idx];
    if ( dist ) *dist = (float) Math::Sqrt( sqdist );
    return true;
}


bool PosInfo::Line2DData::getPos( const Coord& coord,
				  PosInfo::Line2DPos& pos, float maxdist ) const
{
    float dist;
    return getPos(coord,pos,&dist) && dist < maxdist;
}


bool PosInfo::Line2DData::getPos( int nr, PosInfo::Line2DPos& pos ) const
{
    bool found; int idx = gtIndex( nr, found );
    if ( !found ) return false;
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
	const int fac = SI().zDomain().userFactor();
	strm << "Z range " << SI().getZUnitString() << ":\t" << fac*zrg_.start
	     << '\t' << fac*zrg_.stop << "\t" << fac*zrg_.step;
	strm << "\n\nTrcNr\tX-coord\tY-coord" << od_newline;
    }

    for ( int idx=0; idx<posns_.size(); idx++ )
    {
	const PosInfo::Line2DPos& pos = posns_[idx];
	strm << pos.nr_ << '\t' << pos.coord_.x << '\t' << pos.coord_.y << '\n';
    }
    strm.flush();
}


bool PosInfo::Line2DData::read( od_istream& strm, bool asc )
{
    int linesz = -1;
    if ( asc )
	strm >> zrg_.start >> zrg_.stop >> zrg_.step >> linesz;
    else
	strm.getBin( zrg_.start ).getBin( zrg_.stop ).getBin( zrg_.step )
	    .getBin( linesz );


    if ( !strm.isOK() || linesz < 0 )
	return false;

    posns_.erase();
    bendpoints_.erase();
    for ( int idx=0; idx<linesz; idx++ )
    {
	int trcnr = -1;
	if ( asc )
	    strm >> trcnr;
	else
	    strm.getBin( trcnr );
	if ( trcnr<0 || !strm.isOK() )
	    return false;

	PosInfo::Line2DPos pos( trcnr );
	if ( asc )
	    strm >> pos.coord_.x >> pos.coord_.y;
	else
	    strm.getBin( pos.coord_.x ).getBin( pos.coord_.y );

	xrg_.include( pos.coord_.x );
	yrg_.include( pos.coord_.y );
	posns_ += pos;
    }

    return true;
}


bool PosInfo::Line2DData::write( od_ostream& strm, bool asc,
				 bool withnls ) const
{
    const int linesz = posns_.size();
    if ( !asc )
	strm.addBin( zrg_.start ).addBin( zrg_.stop ).addBin( zrg_.step )
	    .addBin( linesz );
    else
    {
	strm << zrg_.start << ' ' << zrg_.stop << ' ' << zrg_.step
	     << ' ' << linesz;
	if ( withnls && linesz ) strm << od_newline;
    }

    for ( int idx=0; idx<linesz; idx++ )
    {
	const PosInfo::Line2DPos& pos = posns_[idx];
	if ( !asc )
	    strm.addBin(pos.nr_).addBin(pos.coord_.x).addBin(pos.coord_.y);
	else
	{
	    BufferString str; str.set( pos.coord_.x );
	    strm << '\t' << pos.nr_ << '\t' << str;
	    str.set( pos.coord_.y );
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
    for ( int idx=0; idx<bendpoints_.size(); idx++ )
	bendpos.add( posns_[bendpoints_[idx]] );
}


const TypeSet<int>& PosInfo::Line2DData::getBendPoints() const
{
    return bendpoints_;
}


void PosInfo::Line2DData::setBendPoints( const TypeSet<int>& bendpoints )
{
    bendpoints_ = bendpoints;
}


StepInterval<Pos::TraceID> PosInfo::Line2DData::trcNrRange() const
{
    const int sz = posns_.size();
    StepInterval<int> res( -1, -1, 1 );
    if ( sz < 1 ) return res;
    res.start = posns_[0].nr_;
    res.stop = posns_[sz-1].nr_;
    if ( sz == 1 ) return res;

    res.step = 0;
    for ( int idx=1; idx<sz; idx++ )
    {
	const int diff = posns_[idx].nr_ - posns_[idx-1].nr_;
	if ( diff < 1 ) continue;
	if ( res.step < 1 || diff < res.step )
	    res.step = diff;
	if ( res.step == 1 )
	    break;
    }
    return res;
}


Coord PosInfo::Line2DData::getNormal( int trcnr ) const
{
    bool found; const int posidx = gtIndex( trcnr, found );
    if ( !found ) return Coord::udf();

    Coord pos = posns_[posidx].coord_;
    Coord v1;
    if ( posidx+1<posns_.size() )
	v1 = posns_[posidx+1].coord_ - pos;
    else if ( posidx-1>=0 )
	v1 = pos - posns_[posidx-1].coord_;

    if ( v1.x == 0 )
	return Coord( 1, 0 );
    else if ( v1.y == 0 )
	return Coord( 0, 1 );
    else
    {
	const double length = Math::Sqrt( v1.x*v1.x + v1.y*v1.y );
	return Coord( -v1.y/length, v1.x/length );
    }
}


float PosInfo::Line2DData::distBetween( int starttrcnr, int stoptrcnr ) const
{
    if ( stoptrcnr < starttrcnr ) return mUdf(float);
    bool found; const int startidx = gtIndex( starttrcnr, found );
    if ( !found ) return mUdf(float);
    const int stopidx = gtIndex( stoptrcnr, found );
    if ( !found ) return mUdf(float);

    float dist = 0.f;
    for ( int idx=startidx; idx<stopidx; idx++ )
	dist += (float) posns_[idx+1].coord_.distTo( posns_[idx].coord_ );
    return dist;
}


void PosInfo::Line2DData::compDistBetwTrcsStats( float& max,
						 float& median ) const
{
    max = 0;
    median = 0;
    double maxsq = 0;
    TypeSet<double> medset;
    const TypeSet<PosInfo::Line2DPos>& posns = positions();
    for ( int pidx=1; pidx<posns.size(); pidx++ )
    {
	const double distsq =
			posns[pidx].coord_.sqDistTo( posns[pidx-1].coord_ );

	if ( !mIsUdf(distsq) && !mIsZero(distsq, 1e-3) )
	{
	    if ( distsq > maxsq )
		maxsq = distsq;
	    medset += distsq;
	}
    }

    if ( medset.size() )
    {
	sort( medset );
	median = mCast( float,
			Math::Sqrt( medset[ mCast(int, medset.size()/2) ] ) );
    }

    max = mCast( float, Math::Sqrt(maxsq) );
}


bool PosInfo::Line2DData::coincidesWith( const PosInfo::Line2DData& oth ) const
{
    const TypeSet<Line2DPos>& mypos = positions();
    const TypeSet<Line2DPos>& othpos = oth.positions();
    if ( mypos.isEmpty() || othpos.isEmpty() )
	return false;

    const int startnr = mMAX( mypos.first().nr_, othpos.first().nr_ );
    const int stopnr = mMIN( mypos.last().nr_, othpos.last().nr_ );
    bool found = false;
    const int mystartidx = gtIndex( startnr, found );
    const int mystopidx = gtIndex( stopnr, found );
    const int othstartidx = oth.gtIndex( startnr, found );
    const int othstopidx = oth.gtIndex( stopnr, found );
    if ( mystartidx < 0 || mystopidx < 0 || othstartidx < 0 || othstopidx < 0 )
	return false;

    int myidx = mystartidx, othidx = othstartidx;
    bool foundcommon = false;
    while ( myidx <= mystopidx && othidx <= othstopidx )
    {
	const int trcnr = mypos[myidx].nr_;
	if ( trcnr == othpos[othidx].nr_ )
	{
	    foundcommon = true;
	    if ( !mIsEqual(mypos[myidx].coord_.x,othpos[othidx].coord_.x,1.0) ||
		 !mIsEqual(mypos[myidx].coord_.y,othpos[othidx].coord_.y,1.0) )
		return false;

	    myidx++; othidx++;
	}
	else if ( trcnr < othpos[othidx].nr_ )
	    myidx++;
	else
	    othidx++;
    }

    return foundcommon;
}
