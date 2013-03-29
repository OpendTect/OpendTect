#ifndef welldahobj_h
#define welldahobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "sets.h"
#include "namedobj.h"
#include "ranges.h"

namespace Well
{

/*!
\brief Depth/Distance along hole object.
*/

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
    Interval<float>	dahRange() const
    			{ return size() ? Interval<float>(dah(0),dah(size()-1)) 					: Interval<float>(0,0); }

    float		dahStep(bool min_else_average) const;

    void		addToDahFrom(int fromidx,float extradah);
    void		removeFromDahFrom(int fromidx,float extradah);

    void		deInterpolate();
    			//!< Remove unnecessary points
    float*              dahArr()                        { return dah_.arr(); }
    const float*        dahArr() const                  { return dah_.arr(); }

protected:

    TypeSet<float>	dah_;

    virtual void	removeAux(int)			= 0;
    virtual void	eraseAux()			= 0;
};


#define mWellDahObjInsertAtDah(dh,v,vals,ascendingvalonly)\
{\
    if ( mIsUdf(v) ) return false;\
    if ( dah_.isEmpty() || dh >= dah_[dah_.size()-1] )\
    {\
	if ( ascendingvalonly && v <= vals[dah_.size()-1] )\
	    return false;\
	dah_ += dh; vals += val;\
    }\
    if ( dh < dah_[0] )\
    {\
	if ( ascendingvalonly && v >= vals[0] )\
	    return false;\
	dah_.insert( 0, dh ); vals.insert( 0, v );\
    }\
    const int insertidx = indexOf( dh );\
    if ( insertidx<0 ) return false;\
    if ( ascendingvalonly && (v <= vals[insertidx] || v >= vals[insertidx+1]) )\
	return false;\
    dah_.insert( insertidx+1, dh ); vals.insert( insertidx+1, v );\
}



}; // namespace Well

#endif

