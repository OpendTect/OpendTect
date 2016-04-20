#ifndef welldahobj_h
#define welldahobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		May 2004
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "namedobj.h"
#include "ranges.h"
#include "sets.h"

namespace Well
{

/*!\brief Depth/Distance along hole object.  */

mExpClass(Well) DahObj : public ::NamedObject
{
public:

			DahObj( const char* nm=0 )
			: ::NamedObject(nm)		{}
			DahObj( const DahObj& d )
			    : ::NamedObject(d.name())
			    , dah_(d.dah_)		{}

    inline int		size() const			{ return dah_.size(); }
    inline float	dah(int idx) const		{ return dah_[idx]; }
    virtual float	value(int idx) const		= 0;
    virtual bool	insertAtDah(float dah, float val) = 0;
    int			indexOf(float dah) const;
    virtual void	remove( int idx )
			{ dah_.removeSingle(idx); removeAux(idx); }
    virtual void	setEmpty()
			{ dah_.erase(); eraseAux(); }
    inline bool		isEmpty() const			{ return size() == 0; }
    Interval<float>	dahRange() const;
			//!< returns (0, 0) for empty dah_

    float		dahStep(bool min_else_average) const;

    void		addToDahFrom(int fromidx,float extradah);
    void		removeFromDahFrom(int fromidx,float extradah);

    void		deInterpolate();
			//!< Remove unnecessary points
    float*              dahArr()                        { return dah_.arr(); }
    const float*        dahArr() const                  { return dah_.arr(); }

protected:

    TypeSet<float>	dah_;

    bool		doInsertAtDah(float dh,float val,TypeSet<float>&,
					bool ascendingvalonly);

    virtual void	removeAux(int)			= 0;
    virtual void	eraseAux()			= 0;
};

}; // namespace Well

#endif
