#ifndef rcol_H
#define rcol_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rcol.h,v 1.1 2004-09-21 12:39:17 kristofer Exp $
________________________________________________________________________

-*/


/*!\brief Object with row and col. */

template <class T> class TypeSet;

class RCol 
{
public:
    virtual		~RCol() {}
    virtual int&	r() 				= 0;
    virtual const int&	r() const			= 0;

    virtual int&	c() 				= 0;
    virtual const int&	c() const			= 0;

    const RCol&		operator+=( const RCol& rc )
			{ r() += rc.r(); c() += rc.c(); return *this; }
    const RCol&		operator-=( const RCol& rc )
			{ r() -= rc.r(); c() -= rc.c(); return *this; }
    const RCol&		operator*=( const RCol& rc )
			{ r() *= rc.r(); c() *= rc.c(); return *this; }
    const RCol&		operator*=( int factor )
			{ r() *= factor; c() *= factor;  return *this; } 
    const RCol&		operator/=( const RCol& rc )
			{ r() /= rc.r(); c() /= rc.c();  return *this; } 

    bool		operator==( const RCol& rc ) const
			{ return r()==rc.r() && c()==rc.c(); }
    bool		operator!=( const RCol& rc ) const
			{ return ! (*this==rc); }

    bool		isNeighborTo( const RCol&, const RCol& step,
	    			      bool eightconnectivity=true ) const;
    			/*!<\returns true if the object is a neighbor with the 
			   provided RCol. The neighborhood is defined with
			   either eight- or four-connectivity */

    float		angleTo(const RCol&) const;
			/*!<\returns the smallest angle between the vector
			      going from 0,0 to the object and the vector
			      going from 0,0 to rc.*/
    float		clockwiseAngleTo(const RCol& rc) const;
    			/*!<\returns the angle between the vector going from
			     0,0 to the object and the vector going from 0,0
			     to rc in the clockwise direction.*/
    float		counterClockwiseAngleTo(const RCol&) const;
			/*!<\returns the angle between the vector going from
			      0,0 to the object and the vector going from 0,0
			      to rc in the counterclockwise direction.*/

    int			distanceSq(const RCol& rc ) const
			{
			    int rdiff = r()-rc.r();
			    int cdiff = c()-rc.c();
			    return rdiff*rdiff+cdiff*cdiff;
			}

    template <class T>
    inline static void 	makeLine( const RCol& start, const RCol& stop,
				  TypeSet<T>& output, const RCol& step );

};


template <class T>
class RColLineBuilder
{
public:
    			RColLineBuilder( const RCol& start,
					   const RCol& dir,
					   const RCol& step,
					   bool stopafterdir,
					   TypeSet<T>& line);
   int			nextStep();

protected:
   float		distToLine( const RCol& rc ) const;
   T			start;
   const T		dir;
   const T		step;
   const float		dirlen;
   bool			stop;
   TypeSet<T>&	line;
};


template <class T> inline
RColLineBuilder<T>::RColLineBuilder( const RCol& start_,
	const RCol& dir_, const RCol& step_, bool stopafterdir,
	TypeSet<T>& line_)
   : start( start_ )
   , dir( dir_ )
   , step( step_ )
   , stop( stopafterdir )
   , line( line_ )
   , dirlen( sqrt(dir_.row*dir_.row+dir_.col*dir_.col))
{}


template <class T> inline
int RColLineBuilder<T>::nextStep()
{
    if ( !dir.r() && !dir.c() )
    {
	pErrMsg("No direction" );
	return -1;
    }

    T bestrc;
    if ( line.size() )
    {
	const T& lastpos = line[line.size()-1];

	float disttoline = mUndefValue;

	if ( dir.row )
	{
	    const T candidate =
	    lastpos+T(dir.r()>0?step.c():-step.r(), 0 );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
	    { bestrc = candidate; disttoline=dist; }
	}

	if ( dir.col )
	{
	    const T candidate =
		lastpos+T(0,dir.col>0?step.col:-step.col );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
	    { bestrc = candidate; disttoline=dist; }
	}

	if ( dir.row && dir.col )
	{
	    const T candidate =
		lastpos+T( dir.row>0?step.col:-step.row,
				dir.col>0?step.col:-step.col );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
	    { bestrc = candidate; disttoline=dist; }
	}
    }
    else
	bestrc = start;

    line += bestrc;
    return stop && bestrc==start+dir? 0 : 1;
}


template <class T>
float RColLineBuilder<T>::distToLine( const RCol& rc ) const
{
    return fabs((dir.r()*(rc.c()-start.c())-dir.c()*(rc.r()-start.r()))/dirlen);
}


template <class T> inline
void RCol::makeLine( const RCol& start, const RCol& stop,
		     TypeSet<T>& output, const RCol& step )
{
    output.erase();
    if ( start==stop )
    { output += start; return; }

    RColLineBuilder<T> builder( start, stop-start, step, true, output );
    while ( builder.nextStep() ) ;
}



#endif
