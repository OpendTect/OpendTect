#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mnemonics.h"
#include "repos.h"

class Property;
class MathProperty;
class uiEditPropRef;


/*!\brief Ref Data for a (usually petrophysical) property.

We prepare for many variants of the name as is not uncommon in practice
(Density, Den, Rho, RhoB, ... you know the drill). The names will be unique
- case insensitive, in the Set. Hence, identity is established case insensitive.

 */

mExpClass(General) PropertyRef : public NamedCallBacker
{
public:

    typedef Mnemonic::StdType StdType;

			PropertyRef(const Mnemonic& = Mnemonic::undef(),
				    const char* nm=nullptr);
			PropertyRef(const PropertyRef&);
    virtual		~PropertyRef();

    virtual PropertyRef* clone() const	{ return new PropertyRef(*this); }

    static PropertyRef* get(const IOPar&,Repos::Source);

    PropertyRef&	operator =(const PropertyRef&);
    bool		operator ==(const PropertyRef&) const;
    bool		operator !=(const PropertyRef&) const;
    bool		matches(const char* nm,bool matchaliases,
				float* matchval =nullptr) const;
    bool		hasFixedDef() const	{ return mathdef_; }
    bool		isElastic() const;
    virtual bool	isElasticForm() const	{ return false; }

    inline const Mnemonic&	mn() const	{ return mn_; }
    inline StdType	stdType() const		{ return mn_.stdType(); }
    inline bool		hasType( StdType t ) const
			{ return mn_.stdType() == t; }
    inline bool		isCompatibleWith( const Mnemonic& mn ) const
			{ return &mn == &mn_; }
    inline bool		isCompatibleWith( const PropertyRef& pr ) const
			{ return hasType(pr.stdType()); }
    void		setFixedDef(const MathProperty*);
				//!< copy will be made
    void		setUnit(const char*);

    void		addAliases(const BufferStringSet&);
    const BufferStringSet aliases() const;
    const MathProperty& fixedDef() const		{ return *mathdef_; }
			//!< be sure hasFixedDef() returns true!

    mExpStruct(General) DispDefs : public Mnemonic::DispDefs
    {
			DispDefs();
			DispDefs(const Mnemonic::DispDefs&);
			~DispDefs();

	DispDefs&	operator =(const DispDefs&);
	bool		operator ==(const DispDefs&) const;
	bool		operator !=(const DispDefs&) const;
	void		copyFrom(const Mnemonic::DispDefs&);

	const Interval<float>& defRange() const override
			{ return typicalrange_; }

	float		commonValue() const override;

	Property*	defval_ = nullptr;

    private:
	bool		setUnit(const char*) override;
	friend class PropertyRef;

    };

    DispDefs		disp_;
    const UnitOfMeasure* unit() const		{ return uom_; }
			//!< For conversions only, do not use to get the label
    CNotifier<PropertyRef,const UnitOfMeasure*> unitChanged;
			//!< Returns the previous unit

    static const PropertyRef& thickness();
		//!< use this always. It has automatic defaults from SI()
    inline bool		isThickness() const	{ return this == &thickness(); }

    static const char*	standardDenStr()	{ return "Density";}
    static const char*	standardDenAliasStr()	{ return "Den";}
    static const char*	standardPVelStr()	{ return "Pwave velocity";}
    static const char*	standardPVelAliasStr()	{ return "PVel";}
    static const char*	standardSVelStr()	{ return "Swave velocity";}
    static const char*	standardSVelAliasStr()	{ return "SVel";}

protected:

    const Mnemonic&	mn_;
    BufferStringSet	propaliases_;
    MathProperty*	mathdef_ = nullptr;
    const UnitOfMeasure* uom_;
    Repos::Source	source_ = Repos::Temp;

    friend class PropertyRefSet;
    friend class uiEditPropRef;

    void		setDefaults();
    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;
    static const Mnemonic* getFromLegacy(const Mnemonic*,const char* propnm);

};



mExpClass(General) PropertyRefSet : public ManagedObjectSet<PropertyRef>
{
public:

    const PropertyRef*	getByName(const char*,bool matchaliases=true) const;
    const PropertyRef*	getByType(PropertyRef::StdType,int occ=0) const;
    const PropertyRef*	getByMnemonic(const Mnemonic&,int occ=0) const;

    PropertyRef*	ensurePresent(const Mnemonic&,const char* nm1,
				      const char* nm2=nullptr,
				      const char* nm3=nullptr);
    bool		ensureHasElasticProps(bool withswave=true,
					      bool withai=false);

    bool		subselect(PropertyRef::StdType,
				  ObjectSet<const PropertyRef>&)const;

private:
			PropertyRefSet();
			mOD_DisableCopy(PropertyRefSet);

    PropertyRef*	getByName(const char*,bool matchaliases=true);
    PropertyRef*	getByType(PropertyRef::StdType,int occ=0);

    PropertyRefSet&	doAdd(PropertyRef*) override;

    void		readFrom(ascistream&,Repos::Source);
    bool		writeTo(ascostream&) const;
    bool		save(Repos::Source) const;

    friend class PropertyRefSetMgr;
    friend class uiManPROPS;

public:

    static const PropertyRef* getByName(const char*,
				    const ObjectSet<const PropertyRef>&,
				    bool matchaliases);

};

mGlobal(General) const PropertyRefSet& PROPS();
inline PropertyRefSet& ePROPS() { return const_cast<PropertyRefSet&>(PROPS()); }


mExpClass(General) PropertyRefSelection : public ObjectSet<const PropertyRef>
{
public:

			PropertyRefSelection(bool with_thickness=true);
			//<! with only thickness, or empty
			PropertyRefSelection(bool with_thickness,
					     const PropertyRef* exclude);
			//<! Filled with relevant items from PROPS()
			PropertyRefSelection(const Mnemonic&);
			//<! Filled with relevant items from PROPS()
			PropertyRefSelection(PropertyRef::StdType);
			//<! Filled with relevant items from PROPS()

    virtual bool	isElasticSel() const		{ return false; }

    virtual void	fillPar(IOPar&) const;
			//<! Only using the PropertyRef names
    virtual bool	usePar(const IOPar&);
			/*<! Only using the PropertyRef names. No erase
			     True if all properties have been found */

    const PropertyRef*	getByName(const char*,bool matchaliases=true) const;
    const PropertyRef*	getByType(PropertyRef::StdType,int occ=0) const;
    const PropertyRef*	getByMnemonic(const Mnemonic&,int occ=0) const;

    PropertyRefSelection subselect(PropertyRef::StdType) const;

    static const char*	sKeyModelProp();

};
