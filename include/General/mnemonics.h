#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood Qadir
 Date:		Aug 2020
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bufstringset.h"
#include "color.h"
#include "enums.h"
#include "manobjectset.h"
#include "namedobj.h"
#include "ranges.h"
#include "uistring.h"

class UnitOfMeasure;


/*!\brief Reference data common to measured logs and simulated properties.

We prepare for many variants of the name as is not uncommon in practice
(Density, PVEL, SVEL, ...). The names will be unique - case insensitive,
in the Set. Hence, identity is established case insensitive.
Aliases are matched with a GlobExpr, so you can add with wildcards and the like.

 */


mExpClass(General) Mnemonic : public NamedObject
{ mODTextTranslationClass(Mnemonic);
public:

    enum StdType	{
			    Ang, Anis, Area, Class, Comp, Den, Dist, ElaRa,
			    ElPot, Fluid, GR, Imp, Perm, Pres, PresGrad,
			    PresWt, Res, Son, Temp, Time, Vel, Volum, Vol,
			    Other
			};
			mDeclareEnumUtils(StdType)
    static StdType	surveyZType(const SurveyInfo* =nullptr);

    enum Scale		{ Linear, Logarithmic };
			mDeclareEnumUtils(Scale)

			Mnemonic(const char* nm=nullptr,StdType=Other);
			Mnemonic(const Mnemonic& mnc);
    virtual		~Mnemonic();

    Mnemonic&		operator =(const Mnemonic&);
    bool		operator ==(const Mnemonic&) const;
    bool		operator !=(const Mnemonic&) const;
    bool		matches(const char* nm,bool matchaliases) const;
    bool		isKnownAs(const char*) const;
    bool		isCompatibleWith(const Mnemonic*) const;
    float		getMatchValue(const char* nm) const;

    inline bool		isUdf() const	{ return *this == undef(); }

private:

    void		setUnit(const char* lbl);

public:

    mExpStruct(General) DispDefs
    {
			DispDefs();
	virtual		~DispDefs();

	bool		operator ==(const DispDefs&) const;
	bool		operator !=(const DispDefs&) const;

	virtual const Interval<float>& defRange() const { return range_; }
	virtual float	commonValue() const;
	const char*	getUnitLbl() const	{ return unitlbl_.buf(); }

	OD::Color	color_;
	Interval<float> range_ = Interval<float>::udf();
	Interval<float> typicalrange_ = Interval<float>::udf();
	Mnemonic::Scale scale_;

    protected:

	BufferString	unitlbl_;

	virtual bool	setUnit(const char*);
			//!< Returns if changed

    private:
	friend void Mnemonic::setUnit(const char*);
    };

    DispDefs		disp_;
    const UnitOfMeasure* unit() const			{ return uom_; }
			//!< For conversions only, do not use to get the label

    inline StdType	stdType() const			{ return stdtype_; }
    inline BufferStringSet&		aliases()	{ return aliases_; }
    inline const BufferStringSet&	aliases() const { return aliases_; }

    inline bool		hasType( StdType t ) const { return stdtype_ == t; }

    static const Mnemonic& distance();
    static const Mnemonic& volume();
    static const Mnemonic& undef();
    static const Mnemonic& defDEN();
    static const Mnemonic& defPVEL();
    static const Mnemonic& defSVEL();
    static const Mnemonic& defDT();
    static const Mnemonic& defDTS();
    static const Mnemonic& defPHI();
    static const Mnemonic& defSW();
    static const Mnemonic& defFracDensity();
    static const Mnemonic& defFracOrientation();
    static const Mnemonic& defAI();
    static const Mnemonic& defSI();
    static const Mnemonic& defVEL(); //For Time-depth work
    static const Mnemonic& defTime(); //TWT

    static const char*	sKeyMnemonic();

protected:

    StdType			stdtype_;
    BufferString		logtypename_;
    BufferStringSet		aliases_;
    const UnitOfMeasure*	uom_ = nullptr;

    friend class		MnemonicSet;
    void			usePar(const IOPar&);
    void			fromString(const char*);
    void			fillPar(IOPar&) const;

};


mExpClass(General) MnemonicSet : public ManagedObjectSet<Mnemonic>
{
public:

    const Mnemonic*	getByName(const char*,bool matchaliases=true) const;
    const Mnemonic&	getGuessed(const UnitOfMeasure*) const;
			//!< first match only, returns undef() is missing
    const Mnemonic&	getGuessed(Mnemonic::StdType,
			       const BufferStringSet* hintnms =nullptr) const;
			//!< first match only, returns undef() is missing
    void		getNames(BufferStringSet&) const;
    const Mnemonic*	getBestGuessedMnemonics(const char*,
					    bool matchaliases = true) const;

private:
			MnemonicSet();

			MnemonicSet(const MnemonicSet&) = delete;
    MnemonicSet&	operator =(const MnemonicSet&) = delete;

    Mnemonic*		getByName(const char*,bool matchaliases=true);

    MnemonicSet&	doAdd(Mnemonic*) override;

    void		readFrom(ascistream&);

    friend class MnemonicSetMgr;
};


mGlobal(General) const MnemonicSet& MNC();
inline MnemonicSet& eMNC()	{ return const_cast<MnemonicSet&>(MNC()); }


mExpClass(General) MnemonicSelection : public ObjectSet<const Mnemonic>
{
public:

			MnemonicSelection();
			MnemonicSelection(const Mnemonic* exclude);
			MnemonicSelection(const Mnemonic::StdType);

    void		getNames(BufferStringSet&) const;
    const Mnemonic*	getByName(const char*,bool matchaliases=true) const;
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static MnemonicSelection	getAllVolumetrics();
    static MnemonicSelection	getAllPorosity();

private:

    static void		getAll(const BufferStringSet&,MnemonicSelection&);
};
