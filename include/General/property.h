#ifndef property_h
#define property_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: property.h,v 1.17 2010-10-06 15:40:31 cvsbert Exp $
________________________________________________________________________


-*/

#include "objectset.h"
class PropertyRef;


/*!\brief A (usually petrophysical) property of some object.

  Its purpose is to provide a value when asked. Some Property types return a
  'random' value when 'avg' is set to false.
  To get a new value, you need to call reset().
 
 */

mClass Property
{
public:

    			Property( const PropertyRef& pr )
			: ref_(pr)		{}
    virtual		~Property()		{}

    inline const PropertyRef& ref() const	{ return ref_; }
    const char*		name() const;

    virtual float	value(bool avg=true) const = 0;
    virtual void	reset()			{}
    virtual bool	canSet() const		{ return false; }
    virtual void	setValue(float) const	{}

    virtual bool	dependsOn(const Property&) const { return false; }

protected:

    const PropertyRef&	ref_;

};


mClass PropertySet
{
public:

    virtual		~PropertySet()		{ erase(); }

    inline int		size() const		{ return props_.size(); }
    inline bool		isEmpty() const		{ return props_.isEmpty(); }
    int			indexOf(const char*,bool matchaliases=false) const;
    inline bool		isPresent( const char* nm, bool ma=false ) const
    			{ return indexOf(nm,ma) >= 0; }
    Property&		get( int idx )		{ return *props_[idx]; }
    const Property&	get( int idx ) const	{ return *props_[idx]; }
    inline const Property* find( const char* nm, bool ma=false ) const
    						{ return fnd(nm,ma); }
    inline Property*	find( const char* nm, bool ma=false )
    						{ return fnd(nm,ma); }

    bool		add(Property*); //!< refuses to add with identical name
    int			set(Property*); //!< add or change into. returns index.
    void		remove(int);
    void		erase()			{ deepErase(props_); }

    void		reset();	//!< clears 'memory'
    bool		prepareEval();	//!< gets properties in usable state
    inline const char*	errMsg() const		{ return errmsg_; }


protected:

    BufferString	errmsg_;
    Property*		fnd(const char*,bool) const;

    ObjectSet<Property>	props_;

};


#endif
