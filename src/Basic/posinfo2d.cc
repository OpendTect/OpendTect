/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "posinfo2d.h"
#include "math2.h"
#include "survinfo.h"
#include "cubesampling.h"
#include <iostream>


PosInfo::Line2DData::Line2DData( const char* lnm )
    : lnm_(lnm)
    , zrg_(SI().sampling(false).zrg)
{
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


int PosInfo::Line2DData::gtIndex( const Coord& coord, double* sqdist ) const
{
    if ( posns_.isEmpty() )
	return -1;

#   define mSqDist(idx) posns_[idx].coord_.sqDistTo( coord )

    int i0 = 0, i1 = posns_.size()-1;
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

    if ( nrrg.includes(posns_[posidx].nr_,false) )
	return posidx;

    double sqd0 = pos.sqDistTo(posns_[nrrg.start].coord_);
    double sqd1 = pos.sqDistTo(posns_[nrrg.stop].coord_);
    return sqd0 < sqd1 ? nrrg.start : nrrg.stop;
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


void PosInfo::Line2DData::dump( std::ostream& strm, bool pretty ) const
{
    if ( !pretty )
	strm << zrg_.start << '\t' << zrg_.stop << '\t' << zrg_.step << '\n';
    else
    {
	strm << lnm_ << '\n';
	const int fac = SI().zDomain().userFactor();
	strm << "Z range " << SI().getZUnitString() << ":\t" << fac*zrg_.start
	     << '\t' << fac*zrg_.stop << "\t" << fac*zrg_.step;
	strm << "\n\nTrcNr\tX-coord\tY-coord" << std::endl;
    }

    for ( int idx=0; idx<posns_.size(); idx++ )
    {
	const PosInfo::Line2DPos& pos = posns_[idx];
	strm << std::fixed << pos.nr_ << '\t' << pos.coord_.x
				      << '\t' << pos.coord_.y << '\n';
    }
    strm.flush();
}


bool PosInfo::Line2DData::read( std::istream& strm, bool asc )
{
    int linesz = -1;
    if ( asc )
	strm >> zrg_.start >> zrg_.stop >> zrg_.step >> linesz;
    else
    {
	float buf[3];
	strm.read( (char*) buf, 3 * sizeof(float) );
	zrg_.start = buf[0];
	zrg_.stop = buf[1];
	zrg_.step = buf[2];
	strm.read( (char*) &linesz, sizeof(int) );
    }


    if ( !strm.good() || linesz < 0 ) 
	return false;

    posns_.erase();
    for ( int idx=0; idx<linesz; idx++ )
    {
	int trcnr = -1;
	if ( asc )
	    strm >> trcnr;
	else
	    strm.read( (char*) &trcnr, sizeof(int) );
	if ( trcnr<0 || strm.bad() || strm.eof() )
	    return false;

	PosInfo::Line2DPos pos( trcnr );
	if ( asc )
	    strm >> pos.coord_.x >> pos.coord_.y;
	else
	{
	    double dbuf[2];
	    strm.read( (char*) dbuf, 2 * sizeof(double) );
	    pos.coord_.x = dbuf[0]; pos.coord_.y = dbuf[1];
	}
	posns_ += pos;
    }

    return true;
}


bool PosInfo::Line2DData::write( std::ostream& strm, bool asc,
				 bool withnls ) const
{
    const int linesz = posns_.size();
    if ( asc )
    {
	strm << zrg_.start << ' ' << zrg_.stop << ' ' << zrg_.step
	     << ' ' << linesz;
	if ( withnls && linesz ) strm << '\n';
    }
    else
    {
	float buf[] = { zrg_.start, zrg_.stop, zrg_.step };
	strm.write( (const char*) buf, 3 * sizeof(float) );
	strm.write( (const char*) &linesz, sizeof(int) );
    }

    for ( int idx=0; idx<linesz; idx++ )
    {
	const PosInfo::Line2DPos& pos = posns_[idx];
	if ( asc )
	{
	    char str[255];
	    getStringFromDouble(0,pos.coord_.x,str);
	    strm << '\t' << pos.nr_
		 << '\t' << str;
	    getStringFromDouble(0,pos.coord_.y,str);
	    strm << '\t' << str;
	    if ( withnls && idx < linesz-1 ) strm << '\n';
	}
	else
	{
	    double dbuf[2];
	    dbuf[0] = pos.coord_.x; dbuf[1] = pos.coord_.y;
	    strm.write( (const char*) &pos.nr_, sizeof(int) );
	    strm.write( (const char*) dbuf, 2 * sizeof(double) );
	}
    }

    return strm.good();
}


StepInterval<int> PosInfo::Line2DData::trcNrRange() const
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


void PosInfo::Line2DData::compDistBetwTrcsStats( float& max,
						 float& median ) const
{
    max = 0;
    median = 0;
    double maxsq;
    TypeSet<double> medset;
    const TypeSet<PosInfo::Line2DPos>& posns = positions();
    for ( int pidx=1; pidx<posns.size(); pidx++ )
    {
	const double distsq =
			posns[pidx].coord_.sqDistTo( posns[pidx-1].coord_ );

	if ( !mIsUdf(distsq) )
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
