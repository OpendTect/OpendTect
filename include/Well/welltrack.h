#ifndef welltrack_h
#define welltrack_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welltrack.h,v 1.9 2004-05-27 10:07:10 bert Exp $
________________________________________________________________________


-*/

#include "welldahobj.h"
#include "position.h"

namespace Well
{
class D2TModel;

class Track : public DahObj
{
public:

			Track( const char* nm=0 )
			: DahObj(nm)			{}
			Track( const Track& t )
			: DahObj("")			{ *this = t; }
    Track&		operator =(const Track&);

    const Coord3&	pos( int idx ) const		{ return pos_[idx]; }
    float		value( int idx ) const		{ return pos_[idx].z; }

    void		addPoint( const Coord& c, float z, float d )
			{ pos_ += Coord3(c,z); dah_ += d; }
    			//!< d must be > all previous. No checks.

    Coord3		getPos(float d_ah) const;
    float		getDahForTVD(float,float prevdah=mUndefValue) const;
    			//!< Non-unique. previous DAH may be helpful

    			// If you know what you're doing:
    Coord3		coordAfterIdx(float d_ah,int) const;
    			//!< Beware: no bounds check on index.

    bool		alwaysDownward() const;
    void		toTime(const D2TModel&);

protected:


    TypeSet<Coord3>	pos_;

    void		removeAux( int idx )		{ pos_.remove(idx); }
    void		eraseAux()			{ pos_.erase(); }

};


}; // namespace Well

#endif
