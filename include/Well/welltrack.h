#ifndef welltrack_h
#define welltrack_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welltrack.h,v 1.6 2004-05-06 16:15:22 bert Exp $
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

    int			size() const			{ return pos_.size(); }
    const Coord3&	pos( int idx ) const		{ return pos_[idx]; }
    float		dah( int idx ) const		{ return dah_[idx]; }

    void		addPoint( const Coord& c, float z, float d )
			{ pos_ += Coord3(c,z); dah_ += d; }
    			//!< d must be > all previous. No checks.

    Coord3		getPos(float d_ah) const;
    float		getDahForTVD(float,float prevdah=mUndefValue) const;
    			//!< Non-unique. previous DAH may be helpful

    			// If you know what you're doing:
    Coord3		coordAfterIdx(float d_ah,int) const;
    			//!< Beware: no bounds check on index.

protected:


    TypeSet<float>	dah_;
    TypeSet<Coord3>	pos_;

};


}; // namespace Well

#endif
