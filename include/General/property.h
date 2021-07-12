#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
________________________________________________________________________


-*/

#include "generalmod.h"
#include "propertyref.h"
#include "mnemonics.h"
#include "factory.h"

class PropertySet;
class Mnemonic;
struct Prop_Thick_Man;


/*!\brief A (usually petrophysical) property of some object.

  Its purpose is to provide a value when asked. Some Property's have a 'memory'.
  These can be cleared using reset(). Most Properties do not return constant
  values. The parameters can be set in EvalOpts.

 */

mExpClass(General) Property : public NamedObject
{ mODTextTranslationClass(Property);
public:
			Property( const char* name,
				  const Mnemonic& mn = Mnemonic::undef() )
			    : NamedObject(name)
			    , lastval_(mUdf(float))
			    , mn_(*const_cast<Mnemonic*>(&mn))
			    , disp_( mn.disp_ )				{}
			Property( const Property& pr )
			    : NamedObject(pr.name())
			    , lastval_(pr.value())
			    , mn_(const_cast<Mnemonic&>(pr.mnem()))
			    , disp_(mn_.disp_)				{}

    virtual Property*	clone() const			= 0;
    static Property*	get(const IOPar&);
    virtual		~Property()			{}

    Property&		operator =(const Property&);
    inline bool		operator ==( const Property& pr ) const
			{ return name() == pr.name(); }
    inline bool		operator !=( const Property& pr ) const
			{ return name() != pr.name(); }

    bool		isEqualTo(const Property&) const;
    virtual bool	isValue() const		{ return false; }

    inline const PropertyRef& ref() const	{ return mn_.propRefType(); }
    inline const Mnemonic&    mnem() const	{ return mn_; }
    bool		hasType( PropertyRef::StdType tp ) const
						{ return mnem().hasType(tp); }
    void		setMnemonic(const Mnemonic& mn);

    virtual void	reset()			     { lastval_ = mUdf(float); }
    virtual bool	init(const PropertySet&) const;
			    //!< clears 'memory' and makes property usable
    virtual uiString	errMsg() const	{ return uiString::emptyString(); }

    virtual bool	isUdf() const			= 0;
    virtual bool	dependsOn(const Property&) const { return false; }

    virtual const char*	type() const			= 0;
    virtual const char*	def() const			= 0;
    virtual void	setDef(const char*)		= 0;
    bool		hasFixedDef() const		{ return mathdef_; }
    const MathProperty& fixedDef() const		{ return *mathdef_; }
    void		setFixedDef(const MathProperty*);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		getDataUsingPar(const IOPar&);

    mDefineFactory2ParamInClass(Property,const char*,const Mnemonic&,factory);

    mExpClass(General) EvalOpts
    {
    public:

	enum ValOpt	{ New, Prev, Avg };

			EvalOpts( ValOpt vo=New, float relpos=0.5 )
			    : valopt_(vo)
			    , relpos_(relpos)
			    , absz_(0.f)
			    , relz_(0.f)	{}
	ValOpt		valopt_;
	float		relpos_;
	float		absz_, relz_;

	inline bool	isAvg() const		{ return valopt_==Avg; }
	inline bool	isPrev() const		{ return valopt_==Prev;}
#	define mPropertyEvalAvg Property::EvalOpts(Property::EvalOpts::Avg)
#	define mPropertyEvalPrev Property::EvalOpts(Property::EvalOpts::Prev)
#	define mPropertyEvalNew(pos) \
				Property::EvalOpts(Property::EvalOpts::New,pos)

    };

    Mnemonic::DispDefs	disp_;
    Property*		defval_ = nullptr;
    float		commonValue() const;

    float		value( EvalOpts eo=EvalOpts() ) const
			{
			    return eo.isPrev() || !mIsUdf(lastval_) ? lastval_
				 : (lastval_ = gtVal(eo));
			}

    static const Property&	thickness();
		//!< use this always. It has automatic defaults from SI()
    inline bool			isThickness() const
					    { return this == &thickness(); }
    static void			setThickness(const Property*);

    bool			isKnownAs(const char*) const;

    static const char*	standardSVelStr()	{ return "Swave velocity";}
    static const char*	standardSVelAliasStr()	{ return "SVel";}
    static const char*	standardPVelStr()	{ return "Pwave velocity";}
    static const char*	standardPVelAliasStr()	{ return "PVel";}

protected:

    Mnemonic&		mn_;		//look at the possibility of const
    mutable float	lastval_;
    MathProperty*	mathdef_ = nullptr;

    virtual float	gtVal(EvalOpts) const		= 0;

};


mExpClass(General) PropertySet
{
public:
			PropertySet()		{}
			PropertySet(const MnemonicSelection&);
						//!< Creates ValueProperty's
			PropertySet( const PropertySet& ps )
						{ *this = ps; }
    virtual		~PropertySet();
    PropertySet&	operator =(const PropertySet&);

    inline int		size() const		{ return props_.size(); }
    inline bool		isEmpty() const		{ return props_.isEmpty(); }
    int			indexOf(const char*,bool matchaliases=false) const;
    inline bool		isPresent( const char* nm, bool ma=false ) const
			{ return indexOf(nm,ma) >= 0; }
    inline bool		isPresent(const Property* pr) const
			{ return props_.isPresent(pr); }
    Property&		get( int idx )		{ return *props_[idx]; }
    const Property&	get( int idx ) const	{ return *props_[idx]; }
    inline const Property* getByName( const char* nm ) const
			{ const int idx = indexOf(nm);
			  return idx < 0 ? 0 : props_[idx]; }
    inline const Property* find( const char* nm, bool ma=false ) const
						{ return fnd(nm,ma); }
    inline Property*	find( const char* nm, bool ma=false )
						{ return fnd(nm,ma); }
    int			indexOf( const Property& p ) const
						{ return props_.indexOf(&p); }
    int			indexOf(PropertyRef::StdType,int occ=0) const;
    void		getPropertiesOfRefType(PropertyRef::StdType,
					   ObjectSet<const Property>&) const;
    static PropertySet	getAll(bool with_thickness=true,
					const Property* exclude=0);

    bool		add(Property*); //!< refuses to add with identical name
    int			set(Property*); //!< add or change into. returns index.
    void		remove(int);
    void		replace(int,Property*);
    void		swap(int idx1,int idx2);
    void		erase()			{ deepErase(props_); }

    bool		prepareUsage() const;	//!< init()'s all Properties
    void		resetMemory();
    inline uiString	errMsg() const		{ return errmsg_; }

    bool		save(Repos::Source) const;
    void		readFrom(ascistream&);
    bool		writeTo(ascostream&) const;

protected:

    ObjectSet<Property>	props_;
    mutable uiString	errmsg_;

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
    static void		initClass() { factory().addCreator(create,typeStr());} \
    static Property*	create( const char* nm, const Mnemonic& m ) \
						{ return new clss(nm,m); } \
    virtual clss*	clone() const		{ return new clss( *this ); }\
    virtual const char*	def() const; \
    virtual void	setDef(const char*); \
    virtual bool	isUdf() const;



/*!\brief Simple, single-value property */

mExpClass(General) ValueProperty : public Property
{
public:
			ValueProperty( const char* nm, const Mnemonic& mnc );
			ValueProperty( const char* nm,
				       const Mnemonic& mnc, float v );
			virtual bool	isValue() const		{ return true; }

    float		val_;

    mDefPropertyFns(ValueProperty,"Value");
    void		setValue( float v )	{ val_ = v; }

};

/*!\brief Range of values.  pos_ is usually in [0,1]. */

mExpClass(General) RangeProperty : public Property
{
public:
			RangeProperty( const char* nm, const Mnemonic& mnc )
			: Property(nm,mnc)
			, rg_(mnc.disp_.range_) {}
			RangeProperty( const Mnemonic& mnc,
				       Interval<float> rg )
			: Property(mnc.name(),mnc)
			, rg_(rg)			{}

    Interval<float>	rg_;

    mDefPropertyFns(RangeProperty,"Range");

protected:

    float		gtAvgVal() const;
};


mGlobal(General) const PropertySet& PROPS();
inline PropertySet& ePROPS() { return const_cast<PropertySet&>(PROPS()); }


mExpClass(General) PropertySelection : public ObjectSet<const Property>
{
public:

			PropertySelection();
    bool		operator ==(const PropertySelection&) const;

    int			indexOf(const char*) const;
    int			find(const char*) const; // also uses 'isKnownAs'

    inline bool		isPresent( const char* prnm ) const
			{ return indexOf( prnm ) >= 0; }
    inline int		indexOf( const Property* pr ) const
			{ return ObjectSet<const Property>::indexOf(pr); }
    inline bool		isPresent( const Property* pr ) const
			{ return ObjectSet<const Property>::isPresent(pr); }

    inline const Property* getByName( const char* nm ) const
			{ const int idx = indexOf(nm);
			  return idx < 0 ? 0 : (*this)[idx]; }

    PropertySelection	subselect(const Mnemonic&) const;

    static PropertySelection getAll(bool with_thickness=true,
					const Property* exclude=0);
    static PropertySelection getAll(const Mnemonic&);

};
