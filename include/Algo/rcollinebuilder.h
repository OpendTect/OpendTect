#ifndef rcollinebuilder_h
#define rcollinebuilder_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris (separated by Bert)
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________


*/

#include "rcol.h"
#include "math2.h"


template <class T>
bool makeLine(const T& start,const T& stop, TypeSet<T>& output,const T& step);


/*!
\brief Creates a line in RowCol space.
*/

template <class T>
class RColLineBuilder
{
public:
    			RColLineBuilder(const T& start,const T& dir,
				        const T& step,TypeSet<T>& line);
   int			nextExtension();
   			/*!<\returns 1 if the extension went well, -1 if
			     	       the direction is zero. */

protected:

   const T&		start_;
   const T&		dir_;
   const T&		step_;
   const float		dirlen_;
   TypeSet<T>&		line_;

   float		distToLine(const T&) const;

};


template <class T> inline
RColLineBuilder<T>::RColLineBuilder( const T& start,
	const T& dir, const T& step, TypeSet<T>& line )
   : start_( start )
   , dir_(dir)
   , step_(step)
   , line_(line)
   , dirlen_(Math::Sqrt(float(dir_[0]*dir_[0]+dir_[1]*dir_[1])))
{ }


template <class T> inline
int RColLineBuilder<T>::nextExtension()
{
    if ( !dir_[0] && !dir_[1] )
	return -1;

    T bestrc;
    if ( line_.size() )
    {
	const T& lastpos = line_[line_.size()-1];

	float disttoline = mUdf(float);

	if ( dir_[0] )
	{
	    const T candidate =
		lastpos + T(dir_[0]>0?step_[0]:-step_[0], 0 );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
		{ bestrc = candidate; disttoline=dist; }
	}

	if ( dir_[1] )
	{
	    const T candidate =
		lastpos + T(0,dir_[1]>0?step_[1]:-step_[1] );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
		{ bestrc = candidate; disttoline=dist; }
	}

	if ( dir_[0] && dir_[1] )
	{
	    const T candidate =
		lastpos + T( dir_[0]>0?step_[0]:-step_[0],
				dir_[1]>0?step_[1]:-step_[1] );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
		{ bestrc = candidate; disttoline=dist; }
	}
    }
    else
	bestrc = start_;

    line_ += bestrc;
    return 1;
}


template <class T> inline
float RColLineBuilder<T>::distToLine( const T& rc ) const
{
    return fabs((dir_[0]*(rc[1]-start_[1])-dir_[1]*(rc[0]-start_[0]))/dirlen_);
}


template <class T> inline
bool makeLine( const T& start, const T& stop, const T& step, TypeSet<T>& output)
{
    if ( start[0]%step[0] != stop[0]%step[0]
      || start[1]%step[1] != stop[1]%step[1] )
	return false;

    output.erase();
    if ( start == stop )
	{ output += T(start); return true; }

    T dir = stop;
    dir -= start;

    RColLineBuilder<T> builder( start, dir, step, output );

    while ( builder.nextExtension()>0 && output[output.size()-1]!=stop )
	;

    return true;
}


#endif
