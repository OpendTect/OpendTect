#ifndef unitofmeasure_h
#define unitofmeasure_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Feb 2004
 RCS:		$Id: unitofmeasure.h,v 1.13 2010-09-24 13:39:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "propertyref.h"
#include "scaler.h"
#include "repos.h"
#include "survinfo.h"

class UnitOfMeasureRepository;

mGlobal UnitOfMeasureRepository& UoMR();


/*!\brief Unit of Measure
 
 Only linear transformations to SI units supported.

 All units of measure in OpendTect are available through the UoMR() instance
 accessor of the singleton UnitOfMeasureRepository instance.

 */

mClass UnitOfMeasure : public NamedObject
{
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

    template <class T>
    T			getSIValue( T inp ) const
    						{ return scaler_.scale(inp); }
    template <class T>
    T			getUserValueFromSI( T inp ) const
						{ return scaler_.unScale(inp); }
    template <class T>
    T			internalValue(T inp) const;
    template <class T>
    T			userValue(T inp) const;

    static const UnitOfMeasure* getGuessed(const char*);
    Repos::Source	source() const			{ return source_; }
    void		setSource( Repos::Source s )	{ source_ = s; }

    static const UnitOfMeasure* surveyDefZUnit();
    static const char*	surveyDefZUnitAnnot(bool symbol,bool withparens);
    static const UnitOfMeasure* surveyDefDepthUnit();
    static const char*	surveyDefDepthUnitAnnot(bool symbol,bool withparens);
    static const char*	zUnitAnnot(bool time,bool symbol,bool withparens);

protected:

    BufferString	symbol_;
    LinScaler		scaler_;
    PropertyRef::StdType proptype_;
    Repos::Source	source_;

};


template <class T> T UnitOfMeasure::internalValue( T inp ) const
{
    if ( SI().zInFeet() )
    {
	if ( strstr(symbol_.buf(),"Feet") )
	    return inp;
	else if ( strstr(symbol_.buf(),"Meter") )
	    return getUserValueFromSI( inp );
    }

    return getSIValue( inp );
}


template <class T> T UnitOfMeasure::userValue( T inp ) const
{
    if ( SI().zInFeet() )
    {
	if ( strstr(symbol_.buf(),"Feet") )
	    return inp;
	else if ( strstr(symbol_.buf(),"Meter") )
	    return getSIValue( inp );
    }

    return getUserValueFromSI( inp );
}


/*!\brief Repository of all Units of Measure in the system.
 
 At first usage of the singleton instance of this class (accessible through
 the global UoMR() function), the data files for the repository are
 searched, by iterating through the different 'Repos' sources (see repos.h).
 Then, the standard ones like 'feet' are added if they are not yet defined
 in one of the files.

 */


mClass UnitOfMeasureRepository
{
public:

    const UnitOfMeasure* get(const char* nm) const;
    			//!< Will try names first, then symbols, otherwise null
    static const char*	guessedStdName(const char*);
    			//!< May return null

    const ObjectSet<const UnitOfMeasure>& all() const	{ return entries; }
    void		getRelevant(PropertyRef::StdType,
	    			    ObjectSet<const UnitOfMeasure>&) const;

    bool		add(const UnitOfMeasure&);
    			//!< returns whether already present
    bool		write(Repos::Source) const;

private:

    			UnitOfMeasureRepository();

    ObjectSet<const UnitOfMeasure> entries;

    void		addUnitsFromFile(const char*,Repos::Source);

    friend UnitOfMeasureRepository& UoMR();

};


#endif
