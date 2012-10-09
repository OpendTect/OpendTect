#ifndef property_h
#define property_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "propertyref.h"
#include "factory.h"

class PropertySet;
class IOPar;


/*!\brief A (usually petrophysical) property of some object.

  Its purpose is to provide a value when asked. Some Property's have a 'memory'.
  These can be cleared using reset(). Most Properties do not return constant
  values. The parameters can be set in EvalOpts.

 */

mClass Property
{
public:

    			Property( const PropertyRef& pr )
			: ref_(pr)			{}
    virtual Property*	clone() const			= 0;
    static Property*	get(const IOPar&);
    virtual		~Property()			{}
    bool		isEqualTo(const Property&) const;

    inline const PropertyRef& ref() const		{ return ref_; }
    const char*		name() const;

    virtual void	reset()				{}
    virtual bool	init(const PropertySet&) const	{ return true; }
			    //!< clears 'memory' and makes property usable
    virtual const char*	errMsg() const			{ return 0; }

    virtual bool	isUdf() const			= 0;
    virtual bool	dependsOn(const Property&) const { return false; }

    virtual const char*	type() const			= 0;
    virtual const char*	def() const			= 0;
    virtual void	setDef(const char*)		= 0;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    mDefineFactory1ParamInClass(Property,const PropertyRef&,factory);

    mClass EvalOpts
    {
    public:

	enum ValOpt	{ New, Prev, Avg };

			EvalOpts( ValOpt vo=New, float relpos=0.5 )
			    : valopt_(vo)
			    , relpos_(relpos)
			    , curz_(0)		{}
	ValOpt		valopt_;
	float		relpos_;
	float		curz_;

	inline bool	isAvg() const		{ return valopt_==Avg; }
	inline bool	isPrev() const		{ return valopt_==Prev;}
#	define mPropertyEvalAvg Property::EvalOpts(Property::EvalOpts::Avg)
#	define mPropertyEvalPrev Property::EvalOpts(Property::EvalOpts::Prev)
#	define mPropertyEvalNew(pos) \
				Property::EvalOpts(Property::EvalOpts::New,pos)

    };

    float		value( EvalOpts eo=EvalOpts() ) const
			{
			    return eo.isPrev() ? lastval_
				 : (lastval_ = gtVal(eo));
			}

protected:

    const PropertyRef&	ref_;
    mutable float	lastval_;

    virtual float	gtVal(EvalOpts) const		= 0;

};


mClass PropertySet
{
public:

    			PropertySet()		{}
			PropertySet(const PropertyRefSelection&);
						//!< Creates ValueProperty's
			PropertySet( const PropertySet& ps )
						{ *this = ps; }
    virtual		~PropertySet()		{ erase(); }
    PropertySet&	operator =(const PropertySet&);

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
    int			indexOf( const Property& p ) const
						{ return props_.indexOf(&p); }
    int			indexOf( const PropertyRef& pr ) const
						{ return indexOf(pr.name()); }
    int			indexOf(PropertyRef::StdType,int occ=0) const;

    bool		add(Property*); //!< refuses to add with identical name
    int			set(Property*); //!< add or change into. returns index.
    void		remove(int);
    void		replace(int,Property*);
    void		erase()			{ deepErase(props_); }

    bool		prepareUsage() const;	//!< init()'s all Properties
    inline const char*	errMsg() const		{ return errmsg_; }


protected:

    ObjectSet<Property>	props_;
    mutable BufferString errmsg_;

    Property*		fnd(const char*,bool) const;

};


// For impl of Property subclasses. gtVal and the last three must be provided.
#define mDefPropertyFns(clss,typstr) \
protected: \
    virtual float	gtVal(EvalOpts) const; \
public: \
    static const char*	typeStr()		{ return typstr; } \
    virtual const char* type() const		{ return typeStr(); } \
    virtual const char* factoryKeyword() const	{ return type(); } \
    static Property*	create( const PropertyRef& r ) { return new clss(r); } \
    static void		initClass() { factory().addCreator(create,typeStr());} \
    virtual Property*	clone() const		{ return new clss( *this ); }\
    virtual const char*	def() const; \
    virtual void	setDef(const char*); \
    virtual bool	isUdf() const



/*!\brief Simple, single-value property */

mClass ValueProperty : public Property
{
public:

    			ValueProperty( const PropertyRef& pr )
			: Property(pr)
			, val_(pr.disp_.range_.center())	{}
    			ValueProperty( const PropertyRef& pr, float v )
			: Property(pr)
			, val_(v)				{}

    float		val_;

    mDefPropertyFns(ValueProperty,"Value");

};

/*!\brief Range of values.  pos_ is usually in [0,1]. */

mClass RangeProperty : public Property
{
public:

    			RangeProperty( const PropertyRef& pr )
			: Property(pr)
			, rg_(pr.disp_.range_)		{}
    			RangeProperty( const PropertyRef& pr,
				       Interval<float> rg )
			: Property(pr)
			, rg_(rg)			{}

    Interval<float>	rg_;

    mDefPropertyFns(RangeProperty,"Range");

};


#endif
