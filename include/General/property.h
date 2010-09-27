#ifndef property_h
#define property_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: property.h,v 1.13 2010-09-27 10:00:11 cvsbert Exp $
________________________________________________________________________


-*/

#include "objectset.h"
class PropertyRef;


/*!\brief A (usually petrophysical) property of some object */

mClass Property
{
public:

    			Property( const PropertyRef& pr )
			: ref_(pr)		{}
    virtual		~Property()		{}

    inline const PropertyRef& ref() const	{ return ref_; }

    virtual float	value() const		= 0;
    virtual bool	canSet() const		{ return false; }
    virtual void	setValue(float) const	{}

    virtual bool	dependsOn(const Property*) const { return false; }

protected:

    const PropertyRef&	ref_;

};


mClass PropertySet : public ObjectSet<Property>
{
public:

    int			indexOf(const char*) const;
    inline bool		isPresent( const char* nm ) const
    			{ return indexOf(nm) >= 0; }
    inline const Property* get( const char* nm ) const	{ return gt(nm); }
    inline Property*	get( const char* nm )		{ return gt(nm); }

    bool		prepareEval();
    inline const char*	errMsg() const			{ return errmsg_; }

protected:

    BufferString	errmsg_;
    Property*		gt(const char*) const;

};


#endif
