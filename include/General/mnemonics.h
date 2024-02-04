#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bufstringset.h"
#include "color.h"
#include "enums.h"
#include "namedobj.h"
#include "ranges.h"
#include "repos.h"

namespace OD { class LineStyle; };
class Settings;
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
			    ElPot, Fluid, Force, GR, Imp, Perm, Pres, PresGrad,
			    PresWt, Res, Son, Temp, Time, Vel, Volum, Vol,
			    Other
			};
			mDeclareEnumUtils(StdType)
    static StdType	surveyZType(const SurveyInfo* =nullptr);

    enum Scale		{ Linear, Logarithmic };
			mDeclareEnumUtils(Scale)

			Mnemonic(const char* nm=nullptr,StdType=Other);
			Mnemonic(const Mnemonic&);
			~Mnemonic();

    static Mnemonic*	getFromTemplate(const Mnemonic&,const char* customname,
					Repos::Source);

    Mnemonic&		operator =(const Mnemonic&);
    bool		operator ==(const Mnemonic&) const;
    bool		operator !=(const Mnemonic&) const;
    bool		matches(const char* nm,bool matchaliases,
				float* matchval =nullptr) const;
    bool		isCompatibleWith(const Mnemonic*) const;
    bool		isTemplate() const	{ return !origin_; }
    const Mnemonic*	getOrigin() const	{ return origin_; }
    const char*		description() const;

    static BufferString		getUserMnemonicsFileName();
    static IOPar		getUserMnemonics();
    // Retrieves the users Mnemonic display overrides from the Mnemonics_%user%
    // file in the current survey folder
    static void			setUserMnemonics(const IOPar&);
    const UnitOfMeasure*	getDisplayInfo(Scale&,
					       Interval<float>&,
					       BufferString&,
					       OD::LineStyle&) const;
   // Will return display settings from the Mnemonics_%user% file in the current
   // survey folder or the global Mnemonics file if no user override defined.
   // Use this function for occasional access, the next version better for
   // repeated access using overrides in IOPar read with getUserMnemonics()
   // function.
    const UnitOfMeasure*	getDisplayInfo(const IOPar&, Scale&,
					       Interval<float>&,
					       BufferString&,
					       OD::LineStyle&) const;

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
    static const Mnemonic& defMD();
    static const Mnemonic& defTVD();
    static const Mnemonic& defTVDSS();
    static const Mnemonic& defTVDSD();
    static const Mnemonic& defTVDGL();

    static const char*	sKeyMnemonic();

protected:

    StdType			stdtype_;
    BufferString		logtypename_;
    BufferStringSet		aliases_;
    const UnitOfMeasure*	uom_ = nullptr;
    Repos::Source		source_ = Repos::Temp;
    const Mnemonic*		origin_ = nullptr;

    friend class		MnemonicSet;
    void			usePar(const IOPar&);
    void			fromString(const char*);
    void			fillPar(IOPar&) const;

public:

    static float	getMatchValue(const char* str,const BufferStringSet&,
				      bool exactmatch,bool hasaltnm =true);
			/*<! Returns the strength of the match between str and
			     the strings in the set, in decreasing order:
			       Case sensitive match with the first string
			     > Case insensitive match with the first string
			     > Case sensitive match with the alternate string
			     > Case insensitive match with the alternate string
			     > Case sensitive match with any string in the set
			     > Case insensitive match with any string in the set
			     > positive regex search with any string in the set
			     If used, the alternative string must be the second
			     item in the set.
			     The match value for a Case sensitive match with
			     any string and below is the length of that string
			 */
};


mExpClass(General) MnemonicSet : public ManagedObjectSet<Mnemonic>
{
public:

    const Mnemonic*	getByName(const char*,bool matchaliases=true) const;
    void		getNames(BufferStringSet&) const;

    void		erase() override;
    Mnemonic*		pop() override;
    Mnemonic*		removeSingle(int,bool keep_order=true) override;
    void		removeRange(int,int) override;
    Mnemonic*		replace(int,Mnemonic*) override;
    Mnemonic*		removeAndTake(int,bool keep_order) override;
    ManagedObjectSetBase<Mnemonic>&	operator -=(Mnemonic*) override;

private:
			MnemonicSet();
			mOD_DisableCopy(MnemonicSet);

    Mnemonic*		getByName(const char*,bool matchaliases=true);

    MnemonicSet&	doAdd(Mnemonic*) override;

    void		readFrom(ascistream&);
    void		setSource(Repos::Source);

    static void		removeCache(const Mnemonic*);

    friend class MnemonicSetMgr;

public:

    static const Mnemonic* getByName(const char*,
					const ObjectSet<const Mnemonic>&,
					bool matchaliases);

};


mGlobal(General) const MnemonicSet& MNC();
inline MnemonicSet& eMNC()	{ return const_cast<MnemonicSet&>(MNC()); }


mExpClass(General) MnemonicSelection : public ObjectSet<const Mnemonic>
{
public:

			MnemonicSelection();
			MnemonicSelection(const Mnemonic* exclude);
			MnemonicSelection(const Mnemonic::StdType);
			MnemonicSelection(const UnitOfMeasure&);

    void		getNames(BufferStringSet&) const;
    const Mnemonic*	getByName(const char*,bool matchaliases=true) const;
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		sort();  // -> sorts mnemonics alphabetically

    static const Mnemonic* getGuessed(const char*,const Mnemonic::StdType,
				      const BufferStringSet* hintnms =nullptr);
    static const Mnemonic* getGuessed(const char*,const UnitOfMeasure*,
				      const BufferStringSet* hintnms =nullptr);

    static MnemonicSelection	getGroupFor(const Mnemonic&);
    static MnemonicSelection	getAllVolumetrics();
    static MnemonicSelection	getAllSaturations();
    static MnemonicSelection	getAllPorosity();

private:

    const Mnemonic*	getGuessed(const char*,
				   const BufferStringSet* hintnms) const;

};
