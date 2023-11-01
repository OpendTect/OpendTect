#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "mnemonics.h"
#include "repos.h"
#include "scaler.h"
#include "survinfo.h"

class UnitOfMeasureRepository;
namespace ZDomain { class Info; }

mGlobal(General) UnitOfMeasureRepository& UoMR();


/*!\brief Unit of Measure

 Only linear transformations to SI units supported.

 All units of measure in OpendTect are available through the UoMR() instance
 accessor of the singleton UnitOfMeasureRepository instance.

 */

mExpClass(General) UnitOfMeasure : public NamedObject
{ mODTextTranslationClass(UnitOfMeasure);
public:
			UnitOfMeasure(const UnitOfMeasure&);

    typedef Mnemonic::StdType PropType;

			~UnitOfMeasure();

    UnitOfMeasure&	operator =(const UnitOfMeasure&);
    bool		isCompatibleWith(const UnitOfMeasure&) const;

    static const UnitOfMeasure* getGuessed(const char*);

    const char*		symbol() const		{ return symbol_.buf(); }
    const char*		getLabel() const;
			//!< Symbol or name if no symbol, for IOPar I/O
    PropType		propType(int idx=0) const;
    int			nrTypes() const		{ return proptypes_.size(); }
    const LinScaler&	scaler() const		{ return scaler_; }
    Repos::Source	source() const		{ return source_; }

    bool		isImperial() const;

    template <class T>
    T			getSIValue( T inp ) const
			{ return ( T ) scaler_.scale(inp); }
    template <class T>
    T			getUserValueFromSI( T inp ) const
			{ return ( T ) scaler_.unScale(inp); }
    template <class T>
    T			internalValue(T inp) const;
    template <class T>
    T			userValue(T inp) const;

    static const UnitOfMeasure* surveyDefZUnit();
				//!<Default unit in displays (ms,m,ft)
    static const UnitOfMeasure* surveyDefZStorageUnit();
				//!<Default unit in storage (s,m,ft)
    static const UnitOfMeasure* surveyDefTimeUnit();
				//!<Default time unit in displays (ms)
    static const UnitOfMeasure* surveyDefTimeStorageUnit();
				//!<Survey storage time unit (s)
    static const UnitOfMeasure* surveyDefDepthUnit();
				//!<Default depth unit in displays (m,ft)
    static const UnitOfMeasure* surveyDefDepthStorageUnit();
				//!<Default depth unit in storage (m,ft)
    static const UnitOfMeasure* surveyDefVelUnit();
				//!<Default velocity unit in displays (m/s,ft/s)
    static const UnitOfMeasure* surveyDefVelStorageUnit();
				//!<Default velocity unit in storage (m/s,ft/s)
    static const UnitOfMeasure* surveyDefSRDUnit();
				//!<Default srd unit in displays (m,ft)
    static const UnitOfMeasure* surveyDefSRDStorageUnit();
				//!<Default srd unit in storage (m,ft)
    static const UnitOfMeasure* surveyDefOffsetUnit();
				/*!<Default offset unit for real datasets (m,ft)
				    Not used for synthetic gathers */
    static const UnitOfMeasure* secondsUnit();
    static const UnitOfMeasure* millisecondsUnit();
    static const UnitOfMeasure* meterUnit();
    static const UnitOfMeasure* meterSecondUnit();
    static const UnitOfMeasure* feetUnit();
    static const UnitOfMeasure* feetSecondUnit();
    static const UnitOfMeasure* radiansUnit();
    static const UnitOfMeasure* degreesUnit();
    static const UnitOfMeasure* zUnit(const ZDomain::Info&,bool storage=true);

    static uiString	surveyDefZUnitAnnot(bool symbol,bool withparens);
    static uiString	surveyDefTimeUnitAnnot(bool symbol,bool withparens);
    static uiString	surveyDefDepthUnitAnnot(bool symbol,bool withparens);
    static uiString	surveyDefVelUnitAnnot(bool symbol,bool withparens);
    static uiString	zUnitAnnot(bool time,bool symbol,bool withparens);

    static IOPar&	currentDefaults();
			//!< just a list of key -> unit of measure
			//!< key can be property name or whatever seems good
    static void		saveCurrentDefaults();
			//!< store as a user setting on Survey level
			//!< this will be done automatically at survey changes

    static BufferString	getUnitLbl(const UnitOfMeasure*,
				   const char* deflbl=nullptr);

private:
			UnitOfMeasure(const char* nm,const char* symb,
				      double shft,double fact,
				      const TypeSet<PropType>&,Repos::Source);

    BufferString	symbol_;
    LinScaler		scaler_;
    TypeSet<PropType>	proptypes_;
    Repos::Source	source_;

    friend class UnitOfMeasureRepository;

public:
			mDeprecated("Should not be used")
			UnitOfMeasure()
			    : source_(Repos::Temp) {}

			mDeprecated("Should not be used")
			UnitOfMeasure( const char* n, const char* s, double f,
				       PropType t=Mnemonic::Other )
			    : NamedObject(n), symbol_(s)
			    , scaler_(0,f), source_(Repos::Temp)	{}

    mDeprecatedDef
    void		setSymbol( const char* s )	{ symbol_ = s; }
    mDeprecatedDef
    void		setScaler( const LinScaler& s ) { scaler_ = s; }
    mDeprecatedDef
    void		setPropType( PropType prtyp )
			{ proptypes_.setEmpty(); proptypes_.add( prtyp ); }
    mDeprecatedDef
    void		setSource( Repos::Source s )	{ source_ = s; }

};


//!> Converts from one unit into another.
//!> Both units may be null (hence the non-member function).
template <class T> void convValue(T& val,
		const UnitOfMeasure* oldunit, const UnitOfMeasure* newunit);

template <class T> inline T getConvertedValue(T val,
		const UnitOfMeasure* oldunit, const UnitOfMeasure* newunit);


/*!\brief Repository of all Units of Measure in the system.

 At first usage of the singleton instance of this class (accessible through
 the global UoMR() function), the data files for the repository are
 searched, by iterating through the different 'Repos' sources (see repos.h).
 Then, the standard ones like 'feet' are added if they are not yet defined
 in one of the files.

 */


mExpClass(General) UnitOfMeasureRepository
{
public:

    typedef UnitOfMeasure::PropType PropType;

    const UnitOfMeasure* get(PropType,const char* nm) const;
    const UnitOfMeasure* get(const char* nm) const;
    static const char*	guessedStdName(const char*);
			//!< May return null

    const ObjectSet<const UnitOfMeasure>& all() const	{ return entries_; }
    void		getRelevant(PropType,
				    ObjectSet<const UnitOfMeasure>&) const;
    const UnitOfMeasure* getCurDefaultFor(const char* key) const;
    const UnitOfMeasure* getInternalFor(PropType) const;
    const UnitOfMeasure* getDefault(const char* key,PropType) const;

    bool		add(const UnitOfMeasure&);
			//!< returns false when already present
    bool		write(Repos::Source) const;

private:

			UnitOfMeasureRepository();

    ManagedObjectSet<const UnitOfMeasure> entries_;

    void		addUnitsFromFile(const char*,Repos::Source);
    const UnitOfMeasure* findBest(const ObjectSet<const UnitOfMeasure>&,
				  const char* nm) const;
			//!< Will try names first, then symbols, otherwise null

    friend mGlobal(General) UnitOfMeasureRepository& UoMR();

};



template <class T> inline T UnitOfMeasure::internalValue( T inp ) const
{
    if ( SI().zInFeet() && (isCompatibleWith(*surveyDefDepthUnit()) ||
			    isCompatibleWith(*surveyDefVelUnit())) )
    {
	if ( name().contains("Feet") )
	    return inp;
	else if ( name().contains("Meter") )
	{
	    const UnitOfMeasure* feetunit = UoMR().get( "Feet" );
	    return feetunit ? feetunit->getUserValueFromSI( inp ) : inp;
	}
    }

    return getSIValue( inp );
}


template <class T> inline T UnitOfMeasure::userValue( T inp ) const
{
    if ( SI().zInFeet() && (isCompatibleWith(*surveyDefDepthUnit()) ||
			    isCompatibleWith(*surveyDefVelUnit())) )
    {
	if ( name().contains("Feet") )
	    return inp;
	else if ( name().contains("Meter") )
	{
	    const UnitOfMeasure* feetunit = UoMR().get( "Feet" );
	    return feetunit ? feetunit->getSIValue( inp ) : inp;
	}
    }

    return getUserValueFromSI( inp );
}


template <class T> void convValue( T& val,
		const UnitOfMeasure* oldunit, const UnitOfMeasure* newunit )
{ val = getConvertedValue( val, oldunit, newunit ); }


template <class T> inline T getConvertedValue( T val,
		const UnitOfMeasure* oldunit, const UnitOfMeasure* newunit )
{
    if ( oldunit == newunit || mIsUdf(val) )
	return val;

    if ( oldunit )
       val = oldunit->internalValue( val );
    if ( newunit )
	val = newunit->userValue( val );

    return val;
}
