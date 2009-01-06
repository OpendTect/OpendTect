#ifndef welldahobj_h
#define welldahobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id: welldahobj.h,v 1.7 2009-01-06 10:57:11 cvsranojay Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "namedobj.h"

namespace Well
{

mClass DahObj : public ::NamedObject
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
    virtual void	remove( int idx )
			{ dah_.remove(idx); removeAux(idx); }
    virtual void	erase()
			{ dah_.erase(); eraseAux(); }
    inline bool		isEmpty() const			{ return size() == 0; }

    float		dahStep(bool min_else_average) const;

    void		addToDahFrom(int fromidx,float extradah);
    void		removeFromDahFrom(int fromidx,float extradah);

    void		deInterpolate();
    			//!< Remove unnecessary points
    float		operator []( int idx ) const	{ return value(idx); }
    			//!< compliance with IdxAble

protected:

    TypeSet<float>	dah_;

    virtual void	removeAux(int)			= 0;
    virtual void	eraseAux()			= 0;
};


}; // namespace Well

#endif
