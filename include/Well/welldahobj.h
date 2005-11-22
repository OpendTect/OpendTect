#ifndef welldahobj_h
#define welldahobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id: welldahobj.h,v 1.3 2005-11-22 07:57:59 cvshelene Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "uidobj.h"

namespace Well
{

class DahObj : public ::UserIDObject
{
public:

			DahObj( const char* nm=0 )
			: ::UserIDObject(nm)		{}
			DahObj( const DahObj& d )
			    : ::UserIDObject(d.name())
			    , dah_(d.dah_)		{}

    inline int		size() const			{ return dah_.size(); }
    inline float	dah(int idx) const		{ return dah_[idx]; }
    virtual float	value(int idx) const		= 0;
    virtual void	remove( int idx )
			{ dah_.remove(idx); removeAux(idx); }
    virtual void	erase()
			{ dah_.erase(); eraseAux(); }

    float		dahStep(bool min_else_average) const;

    void		addToDahFrom(int fromidx,float extradah);
    void		removeFromDahFrom(int fromidx,float extradah);

protected:

    TypeSet<float>	dah_;

    virtual void	removeAux(int)			= 0;
    virtual void	eraseAux()			= 0;
};


}; // namespace Well

#endif
