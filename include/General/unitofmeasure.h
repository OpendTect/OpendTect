#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Feb 2004
________________________________________________________________________

-*/

#include "generalmod.h"
#include "propertyref.h"
#include "scaler.h"
#include "repos.h"
#include "survinfo.h"

class UnitOfMeasureRepository;

mGlobal(General) UnitOfMeasureRepository& UoMR();


/*!\brief Unit of Measure

 Only linear transformations to SI units supported.

 All units of measure in OpendTect are available through the UoMR() instance
 accessor of the singleton UnitOfMeasureRepository instance.

 */

mExpClass(General) UnitOfMeasure : public NamedObject
{ mODTextTranslationClass(UnitOfMeasure);
public:

			UnitOfMeasure()
			    : proptype_(PropertyRef::Other)
			    , source_(Repos::Temp) {}
			UnitOfMeasure( const char* n, const char* s, double f,
				      PropertyRef::StdType t=PropertyRef::Other)
			    : NamedObject(n), symbol_(s)
			    , scaler_(0,f), source_(Repos::Temp)
			    , proptype_(t)	{}
			UnitOfMeasure( const UnitOfMeasure& uom )
			    : NamedObject(uom.name())
						{ *this = uom; }
    UnitOfMeasure&	operator =(const UnitOfMeasure&);

    const char*		symbol() const		{ return symbol_.buf(); }
    PropertyRef::StdType propType() const	{ return proptype_; }
    const LinScaler&	scaler() const		{ return scaler_; }

    void		setSymbol( const char* s )
						{ symbol_ = s; }
    void		setScaler( const LinScaler& s )
						{ scaler_ = s; }
    void		setPropType( PropertyRef::StdType t )
						{ proptype_ = t; }

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

    static const UnitOfMeasure* getGuessed(const char*);
    Repos::Source	source() const			{ return source_; }
    void		setSource( Repos::Source s )	{ source_ = s; }

    static const UnitOfMeasure* surveyDefZUnit();
				//!<Default unit in displays (ms,m,ft)
    static const UnitOfMeasure* surveyDefZStorageUnit();
				//!<Default unit in storage (s,m,ft)
    static const UnitOfMeasure* surveyDefTimeUnit();
				//!<Default time unit in displays (ms)
    static const UnitOfMeasure* surveyDefDepthUnit();
				//!<Default depth unit in displays (m,ft)
    static const UnitOfMeasure* surveyDefDepthStorageUnit();
				//!<Default depth unit in storage (m,ft)
    static const UnitOfMeasure* surveyDefVelUnit();
				//!<Default velocity unit in displays (m/s,ft/s)
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

protected:

    BufferString	symbol_;
    LinScaler		scaler_;
    PropertyRef::StdType proptype_;
    Repos::Source	source_;

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

    const UnitOfMeasure* get(PropertyRef::StdType,const char* nm) const;
    const UnitOfMeasure* get(const char* nm) const;
    static const char*	guessedStdName(const char*);
			//!< May return null

    const ObjectSet<const UnitOfMeasure>& all() const	{ return entries; }
    void		getRelevant(PropertyRef::StdType,
				    ObjectSet<const UnitOfMeasure>&) const;
    const UnitOfMeasure* getCurDefaultFor(const char* key) const;
    const UnitOfMeasure* getInternalFor(PropertyRef::StdType) const;
    const UnitOfMeasure* getDefault(const char* key,PropertyRef::StdType) const;

    bool		add(const UnitOfMeasure&);
			//!< returns false when already present
    bool		write(Repos::Source) const;

private:

			UnitOfMeasureRepository();

    ManagedObjectSet<const UnitOfMeasure> entries;

    void		addUnitsFromFile(const char*,Repos::Source);
    const UnitOfMeasure* findBest(const ObjectSet<const UnitOfMeasure>&,
				  const char* nm) const;
			//!< Will try names first, then symbols, otherwise null

    friend mGlobal(General) UnitOfMeasureRepository& UoMR();

};



template <class T> inline T UnitOfMeasure::internalValue( T inp ) const
{
    if ( SI().zInFeet() )
    {
	if ( symbol_.contains("ft") )
	    return inp;
	else if ( symbol_.contains("m") )
	{
	    const UnitOfMeasure* feetunit = UoMR().get( "ft" );
	    return feetunit ? feetunit->getUserValueFromSI( inp ) : inp;
	}
    }

    return getSIValue( inp );
}


template <class T> inline T UnitOfMeasure::userValue( T inp ) const
{
    if ( SI().zInFeet() )
    {
	if ( symbol_.contains("ft") )
	    return inp;
	else if ( symbol_.contains("m") )
	{
	    const UnitOfMeasure* feetunit = UoMR().get( "ft" );
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

