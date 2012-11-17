#ifndef polylinend_h
#define polylinend_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		Dec 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "math.h"
#include "position.h"
#include "typeset.h"


/*!\brief (Closed) sequence(s) of connected n-D coordinates.
          Undefined coordinates separate consecutive sequences. */

template <class T>
class PolyLineND : public TypeSet<T>
{
public:
		PolyLineND(bool closed=false)
		    : closed_( closed )					{}

		PolyLineND(const TypeSet<T>& coords,bool closed=false)
		    : TypeSet<T>( coords )
		    , closed_( closed )					{}
		    
    void	setClosed(bool yn)			{ closed_ = yn; }
    bool	isClosed() const			{ return closed_; }

    double	distTo(const T& pt,int* segmentidxptr=0,
		       double* fractionptr=0) const;

    T		closestPoint(const T& pt) const;

    double	arcLength(int idx,double frac=0.0) const;
    double	arcLength(const T& pt) const;
    double	arcLength() const;

    T		getPoint(int idx,double frac=0.0) const;
    T		getPoint(double arclen) const;

protected:

    int		nextIdx(int) const;
    bool	closed_;
};


template <class T> inline
int PolyLineND<T>::nextIdx( int idx ) const
{
    if ( idx<0 || idx>=this->size() || !(*this)[idx].isDefined() )
	return mUdf(int);

    int idy = idx + 1;
    if ( idy>=this->size() || !(*this)[idy].isDefined() )
    {
	while ( (--idy)>=0 && (*this)[idy].isDefined() );

	idy++;

	if ( !closed_ && idy!=idx )
	    return mUdf(int);
    }

    return idy;
}


/* Point-to-segment distance:

	     . pt
	    /|\
	   / | \
	  a  h  b
	 /   |   \
  [idx] /_d__|____\ [idx+1]
	<----c---->      
*/

#define mUpdateMinDist( dist, idx, frac ) \
    if ( dist<mindist ) \
    { \
	mindist = dist; \
	if ( segmentidxptr ) \
	    *segmentidxptr = idx; \
	if ( fractionptr ) \
	    *fractionptr = frac; \
    }

template <class T> inline
double PolyLineND<T>::distTo( const T& pt, int* segmentidxptr,
			      double* fractionptr ) const
{
    double mindist = MAXDOUBLE;

    for ( int idx=0; idx<this->size(); idx++ )
    {
	const int idy = nextIdx( idx );
	if ( mIsUdf(idy) )
	     continue;

	const double a2 = pt.sqDistTo( (*this)[idx] );
	const double b2 = pt.sqDistTo( (*this)[idy] );
	const double c2 = (*this)[idx].sqDistTo( (*this)[idy] );

	if ( c2<=0.0 || b2>=a2+c2 )
	{
	    const double a = Math::Sqrt( a2 );
	    mUpdateMinDist( a, idx, 0.0 );
	}
	else if ( a2 >= b2+c2 )
	{
	    const double b = Math::Sqrt( b2 );
	    mUpdateMinDist( b, idx, 1.0 );
	}
	else
	{
	    const double c = Math::Sqrt( c2 );
	    const double d = (a2+c2-b2) / (2*c);
	    const double h = (a2-d*d)>0 ? Math::Sqrt(a2-d*d) : 0.0;
	    mUpdateMinDist( h, idx, d/c );
	}
    }

    return mindist==MAXDOUBLE ? mUdf(double) : mindist;
}


template <class T> inline
T PolyLineND<T>::closestPoint( const T& pt ) const
{
    int idx;
    double frac;
    if ( mIsUdf( distTo(pt,&idx,&frac) ) )
	return T::udf();

    const int idy = nextIdx( idx );
    if ( mIsUdf(idy) )
	return T::udf();

    return (*this)[idx]*(1.0-frac) + (*this)[idy]*frac;
}


template <class T> inline
double PolyLineND<T>::arcLength( int index, double frac ) const
{
    if ( index<0 || index>=this->size() || frac<0.0 || frac>1.0 )
	return mUdf(double);

    double arclen = 0.0;

    for ( int idx=0; idx<=index; idx++ )
    {
	const int idy = nextIdx( idx );
	if ( mIsUdf(idy) )
	     continue;

	const double len = (*this)[idx].distTo( (*this)[idy] );
	if ( idx==index )
	    return arclen + len * frac;

	arclen += len;
    }

    return arclen;
}


template <class T> inline
double PolyLineND<T>::arcLength( const T& pt ) const
{
    int idx;
    double frac;
    if ( mIsUdf( distTo(pt,&idx,&frac) ) )
	return mUdf(double);

    return arcLength( idx, frac );
}


template <class T> inline
double PolyLineND<T>::arcLength() const
{
    return arcLength( this->size()-1, 1.0 );
}


template <class T> inline
T PolyLineND<T>::getPoint( int idx, double frac ) const
{
    if ( idx<0 || idx>=this->size() || frac<0.0 || frac>1.0 )
	return T::udf();

    if ( frac == 0.0 )
	return (*this)[idx];

    if ( frac==1.0 && !(*this)[idx].isDefined() )
	return getPoint( idx+1, 0.0 );

    const int idy = nextIdx( idx );
    if ( mIsUdf(idy) )
	return T::udf();

    return (*this)[idx]*(1.0-frac) + (*this)[idy]*frac;
}


template <class T> inline
T PolyLineND<T>::getPoint( double arclen ) const
{
    if ( arclen<0.0 )
	return T::udf();

    for ( int idx=0; idx<this->size(); idx++ )
    {
	const int idy = nextIdx( idx );
	if ( mIsUdf(idy) )
	     continue;

	const double len = (*this)[idx].distTo( (*this)[idy] );
	if ( arclen <= len )
	{
	    const double frac = arclen / len;
	    return (*this)[idx]*(1.0-frac) + (*this)[idy]*frac;
	}

	arclen -= len;
    }

    return T::udf();
}


#endif
