#ifndef property_h
#define property_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: property.h,v 1.21 2010-10-20 13:06:16 cvsbert Exp $
________________________________________________________________________


-*/

#include "propertyref.h"
#include "factory.h"


/*!\brief A (usually petrophysical) property of some object.

  Its purpose is to provide a value when asked. Some Property's have a 'memory'.
  These can be cleared using reset(). Some Properties do not return constant
  values. The parameters can be set in EvalOpts.

 */

mClass Property
{
public:

    			Property( const PropertyRef& pr )
			: ref_(pr)			{}
    virtual		~Property()			{}

    inline const PropertyRef& ref() const		{ return ref_; }
    const char*		name() const;

    virtual bool	isUdf() const			= 0;
    virtual void	reset() const			{}

    virtual bool	dependsOn(const Property&) const { return false; }

    virtual const char*	type() const			= 0;
    virtual const char*	def() const			= 0;
    virtual void	setDef(const char*)		= 0;
    mDefineFactory1ParamInClass(Property,const PropertyRef&,factory);

    mClass EvalOpts
    {
    public:
			EvalOpts( bool avg=false, float relpos=0 )
			    : average_(avg)
			    , relpos_(relpos)		{}
	bool		average_;
	float		relpos_;
    };
    virtual float	value(EvalOpts eo=EvalOpts()) const = 0;

protected:

    const PropertyRef&	ref_;

};

// For impl of Property subclasses. The last four must be provided.
#define mDefPropertyFns(clss,typstr) \
    static const char*	typeStr()		{ return typstr; } \
    virtual const char* type() const		{ return typeStr(); } \
    static Property*	create( const PropertyRef& r ) { return new clss(r); } \
    static void		initClass() { factory().addCreator(create,typeStr());} \
    virtual const char*	def() const; \
    virtual void	setDef(const char*); \
    virtual bool	isUdf() const; \
    virtual float	value(EvalOpts eo=EvalOpts()) const


mClass PropertySet
{
public:

    			PropertySet()		{}
			PropertySet(const PropertyRefSelection&);
						//!< Creates ValueProperty's
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
    int			indexOf( const PropertyRef& pr ) const
						{ return indexOf(pr.name()); }
    int			indexOf(PropertyRef::StdType,int occ=0) const;

    bool		add(Property*); //!< refuses to add with identical name
    int			set(Property*); //!< add or change into. returns index.
    void		remove(int);
    void		replace(int,Property*);
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
