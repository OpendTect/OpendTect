#ifndef welltrack_h
#define welltrack_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welltrack.h,v 1.12 2006-01-10 13:17:07 cvshelene Exp $
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
    int			nrPoints() const		{ return pos_.size(); }

    int			insertPoint(const Coord&,float z);
    			//!< a 'good' place will be found
    			//!< If inserted at top, z will be used as first dah
    			//!< returns position index of the new point
    void		addPoint(const Coord&,float z,float dah=mUdf(float));
    			//!< Point must be further down track. No checks.
    void		setPoint(int,const Coord&,float z);
    			//!< Will correct all dahs below point
    void		insertAfterIdx(int,const Coord3&);
    			//!< Know what you're doing - not used normally
    void		removePoint(int);
    			//!< Will correct all dahs below point

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
