#ifndef welltrack_h
#define welltrack_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welltrack.h,v 1.1 2003-08-15 11:12:15 bert Exp $
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

    int			nrPoints() const		{ return pos_.size(); }
    Coord3		pos( int idx ) const		{ return pos_[idx]; }
    float		dah( int idx ) const		{ return dah_[idx]; }

    void		addPoint( const Coord& c, float z, float d )
			{ pos_ += Coord3(c,z); dah_ += d; }
    			//!< d must be > all previous. No checks.

    Coord3		getPos(float d_ah) const;

protected:


    TypeSet<float>	dah_;
    TypeSet<Coord3>	pos_;

};


}; // namespace Well

#endif
