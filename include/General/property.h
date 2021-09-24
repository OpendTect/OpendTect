#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
________________________________________________________________________


-*/

#include "factory.h"
#include "propertyref.h"

class PropertySet;


/*!\brief A (usually petrophysical) property of some object.

  Its purpose is to provide a value when asked. Some Property's have a 'memory'.
  These can be cleared using reset(). Most Properties do not return constant
  values. The parameters can be set in EvalOpts.

 */

mExpClass(General) Property : public CallBacker
{ mODTextTranslationClass(Property);
public:
			Property(const PropertyRef&);
    virtual		~Property();

    virtual Property*	clone() const			= 0;
    static Property*	get(const IOPar&);

    bool		isEqualTo(const Property&) const;
    bool		matches(const char* nm,bool matchaliases) const;
    virtual bool	isValue() const			{ return false; }
    virtual bool	isRange() const			{ return false; }
    virtual bool	isFormula() const		{ return false; }

    inline const PropertyRef& ref() const		{ return ref_; }
    inline const Mnemonic& mn() const		{ return ref_.mn(); }
    inline const UnitOfMeasure* unit() const	{ return ref_.unit(); }
    const char*		name() const;

    virtual void	reset()			     { lastval_ = mUdf(float); }
    virtual bool	init(const PropertySet&) const;
			    //!< clears 'memory' and makes property usable
    virtual uiString	errMsg() const	{ return uiString::emptyString(); }

    virtual bool	isUdf() const			= 0;
    virtual bool	dependsOn(const Property&) const { return false; }

    virtual const char*	type() const			= 0;
    virtual const char*	def() const			= 0;
    virtual void	setDef(const char*)		= 0;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    mDefineFactory1ParamInClass(Property,const PropertyRef&,factory);

    mExpClass(General) EvalOpts
    {
    public:

	enum ValOpt	{ New, Prev, Avg };

			EvalOpts( ValOpt vo=New, float relpos=0.5f )
			    : valopt_(vo)
			    , relpos_(relpos)	{}
	ValOpt		valopt_;
	float		relpos_ = 0.5f;
	float		absz_ = 0.f;
	float		relz_ = 0.f;

	inline bool	isAvg() const		{ return valopt_==Avg; }
	inline bool	isPrev() const		{ return valopt_==Prev;}
#	define mPropertyEvalAvg Property::EvalOpts(Property::EvalOpts::Avg)
#	define mPropertyEvalPrev Property::EvalOpts(Property::EvalOpts::Prev)
#	define mPropertyEvalNew(pos) \
				Property::EvalOpts(Property::EvalOpts::New,pos)

    };

    float		value( EvalOpts eo=EvalOpts() ) const
			{
			    return eo.isPrev() || !mIsUdf(lastval_) ? lastval_
				 : (lastval_ = gtVal(eo));
			}

protected:

    const PropertyRef&	ref_;
    mutable float	lastval_ = mUdf(float);

    virtual float	gtVal(EvalOpts) const		= 0;
    virtual void	doUnitChange(const UnitOfMeasure* olduom,
				     const UnitOfMeasure* newuom);
private:

    void		unitChangedCB(CallBacker*);

};


mExpClass(General) PropertySet : public ObjectSet<Property>
{
public:
			PropertySet();
			PropertySet(const PropertySet&);
			PropertySet(const PropertyRefSelection&);
			//!< Creates ValueProperty's
    virtual		~PropertySet();

    PropertySet&	operator =(const PropertySet&);

    const Property*	getByName(const char*,bool matchaliases=false) const;
    Property*		getByName(const char*,bool matchaliases=false);

    Property*		set(Property*); //!< add or change into
    void		erase() override		{ deepErase(*this); }

    bool		prepareUsage() const;	//!< init()'s all Properties
    void		resetMemory();
    inline uiString	errMsg() const		{ return errmsg_; }

protected:

    mutable uiString	errmsg_;

private:


    PropertySet&	doAdd(Property*) override;
			//!< refuses to add with identical name

};


// For impl of Property subclasses. gtVal and the last three must be provided.
#define mDefPropertyFns(clss,typstr) \
protected: \
    virtual float	gtVal(EvalOpts) const; \
public: \
    static const char*	typeStr()		{ return typstr; } \
    virtual const char* type() const		{ return typeStr(); } \
    virtual const char* factoryKeyword() const	{ return type(); } \
    static Property*	create( const PropertyRef& pr ) \
						{ return new clss(pr); } \
    virtual clss*	clone() const		{ return new clss( *this ); }\
    static void		initClass() { factory().addCreator(create,typeStr());} \
    virtual const char*	def() const; \
    virtual void	setDef(const char*); \
    virtual bool	isUdf() const;



/*!\brief Simple, single-value property */

mExpClass(General) ValueProperty : public Property
{
public:
			ValueProperty(const PropertyRef&);
			ValueProperty(const PropertyRef&,float val);
			~ValueProperty();

    bool		isValue() const override	{ return true; }

    float		val_;

    mDefPropertyFns(ValueProperty,"Value");
    void		setValue( float v )	{ val_ = v; }

private:

    void		doUnitChange(const UnitOfMeasure* olduom,
				     const UnitOfMeasure* newuom) override;

};

/*!\brief Range of values.  pos_ is usually in [0,1]. */

mExpClass(General) RangeProperty : public Property
{
public:
			RangeProperty(const PropertyRef&);
			RangeProperty(const PropertyRef&,
				      const Interval<float>&);
			~RangeProperty();

    bool		isRange() const			{ return true; }

    Interval<float>	rg_;

    mDefPropertyFns(RangeProperty,"Range");

protected:

    float		gtAvgVal() const;

private:

    void		doUnitChange(const UnitOfMeasure* olduom,
				     const UnitOfMeasure* newuom) override;

};
