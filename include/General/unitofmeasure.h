#ifndef unitofmeasure_h
#define unitofmeasure_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Feb 2004
 RCS:		$Id: unitofmeasure.h,v 1.1 2004-02-19 14:02:53 bert Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "property.h"
#include "scaler.h"

class UnitOfMeasureRepository;

UnitOfMeasureRepository& UoMR();


/*!\brief Unit of Measure
 
 Only linear transformations to SI units supported.

 */

class UnitOfMeasure : public UserIDObject
{
public:

    			UnitOfMeasure() : proptype_(PropertyRef::Other)
						{}
    			UnitOfMeasure( const char* n, const char* s, double f,
				      PropertyRef::StdType t=PropertyRef::Other)
			    : UserIDObject(n), symbol_(s)
			    , scaler_(0,f), proptype_(t)
						{}

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
    T			internalValue( T inp ) const
    						{ return scaler_.scale(inp); }
    template <class T>
    T			userValue( T inp ) const
						{ return scaler_.unScale(inp); }

    static const UnitOfMeasure* getGuessed(const char*);

protected:

    BufferString	symbol_;
    LinScaler		scaler_;
    PropertyRef::StdType proptype_;
    

};


/*!\brief Unit of Measure
 
 Only linear transformations to SI units supported.

 */


class UnitOfMeasureRepository
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
    			//!< Note that add is temporary for this run of OD

private:

    			UnitOfMeasureRepository();

    ObjectSet<const UnitOfMeasure> entries;

    void		addUnitsFromFile(const char*);

    friend UnitOfMeasureRepository& UoMR();

};


#endif
