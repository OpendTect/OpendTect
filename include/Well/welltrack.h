#ifndef welltrack_h
#define welltrack_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welltrack.h,v 1.5 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "position.h"

namespace Well
{

class Track
{
public:

			Track()				{}

    int			size() const		{ return pos_.size(); }
    const Coord3&	pos( int idx ) const		{ return pos_[idx]; }
    float		dah( int idx ) const		{ return dah_[idx]; }

    void		addPoint( const Coord& c, float z, float d )
			{ pos_ += Coord3(c,z); dah_ += d; }
    			//!< d must be > all previous. No checks.

    Coord3		getPos(float d_ah) const;
    float		getDahForTVD(float,float prevdah=mUndefValue) const;
    			//!< Non-unique. previous DAH may be helpful

protected:


    TypeSet<float>	dah_;
    TypeSet<Coord3>	pos_;

};


}; // namespace Well

#endif
