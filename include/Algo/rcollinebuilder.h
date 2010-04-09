#ifndef rcollinebuilder_h
#define rcollinebuilder_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris (separated by Bert)
 Date:		Apr 2010
 RCS:		$Id: rcollinebuilder.h,v 1.1 2010-04-09 07:59:04 cvsbert Exp $
________________________________________________________________________


*/

#include "rcol.h"
#include "math2.h"


template <class OutT>
bool makeLine(const RCol& start,const RCol& stop,
	      TypeSet<OutT>& output,const RCol& step);


template <class OutT>
class RColLineBuilder
{
public:
    			RColLineBuilder(const RCol& start,const RCol& dir,
				        const RCol& step,TypeSet<OutT>& line);
   int			nextExtension();
   			/*!<\returns 1 if the extension went well, -1 if
			     	       the direction is zero. */

protected:

   const RCol&		start_;
   const RCol&		dir_;
   const RCol&		step_;
   const float		dirlen_;
   TypeSet<OutT>&	line_;

   float		distToLine(const RCol&) const;

};


template <class OutT> inline
RColLineBuilder<OutT>::RColLineBuilder( const RCol& start,
	const RCol& dir, const RCol& step, TypeSet<OutT>& line )
   : start_( start )
   , dir_(dir)
   , step_(step)
   , line_(line)
   , dirlen_(Math::Sqrt(float(dir_[0]*dir_[0]+dir_[1]*dir_[1])))
{
}


template <class OutT> inline
int RColLineBuilder<OutT>::nextExtension()
{
    if ( !dir_[0] && !dir_[1] )
	return -1;

    OutT bestrc;
    if ( line_.size() )
    {
	const OutT& lastpos = line_[line_.size()-1];

	float disttoline = mUdf(float);

	if ( dir_[0] )
	{
	    const OutT candidate =
		lastpos + OutT(dir_[0]>0?step_[0]:-step_[0], 0 );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
		{ bestrc = candidate; disttoline=dist; }
	}

	if ( dir_[1] )
	{
	    const OutT candidate =
		lastpos + OutT(0,dir_[1]>0?step_[1]:-step_[1] );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
		{ bestrc = candidate; disttoline=dist; }
	}

	if ( dir_[0] && dir_[1] )
	{
	    const OutT candidate =
		lastpos + OutT( dir_[0]>0?step_[0]:-step_[0],
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


template <class OutT> inline
float RColLineBuilder<OutT>::distToLine( const RCol& rc ) const
{
    return fabs((dir_[0]*(rc[1]-start_[1])-dir_[1]*(rc[0]-start_[0]))/dirlen_);
}


template <class OutT> inline
bool makeLine( const RCol& start, const RCol& stop, const RCol& step,
	       TypeSet<OutT>& output )
{
    if ( start[0]%step[0] != stop[0]%step[0]
      || start[1]%step[1] != stop[1]%step[1] )
	return false;

    output.erase();
    if ( start == stop )
	{ output += OutT(start); return true; }

    OutT dir = stop;
    dir -= start;

    RColLineBuilder<OutT> builder( start, dir, step, output );

    while ( builder.nextExtension()>0 && output[output.size()-1]!=stop )
	;

    return true;
}


#endif
