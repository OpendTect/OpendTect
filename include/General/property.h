#ifndef property_h
#define property_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: property.h,v 1.12 2010-09-24 13:39:22 cvsbert Exp $
________________________________________________________________________


-*/

#include "enums.h"
class PropertyRef;


/*!\brief A (usually petrophysical) property of some object */

mClass Property
{
public:

    			Property( const PropertyRef& pr )
			: ref_(pr)		{}
    virtual		~Property()		{}

    const PropertyRef&	ref() const		{ return ref_; }

    virtual float	value() const		= 0;
    virtual bool	canSet() const		{ return false; }
    virtual void	setValue(float) const	{}

    virtual bool	dependsOn(const Property*) const { return false; }

protected:

    const PropertyRef&	ref_;

};


#endif
