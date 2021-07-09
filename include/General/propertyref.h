#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
________________________________________________________________________


-*/

#include "generalmod.h"
#include "generalmod.h"
#include "ranges.h"
#include "namedobj.h"
#include "enums.h"
#include "bufstringset.h"
#include "color.h"
#include "repos.h"


class ascistream;
class ascostream;
class Property;
class MathProperty;
class Mnemonic;
struct PropRef_ThickRef_Man;


/*!\brief Ref Data for a (usually petrophysical) property.

We prepare for many variants of the name as is not uncommon in practice
(Density, Den, Rho, RhoB, ... you know the drill). The names will be unique
- case insensitive, in the Set. Hence, identity is established case insensitive.
Aliases are matched with a GlobExpr, so you can add with wildcards and the like.

 */

mExpClass(General) PropertyRef : public NamedObject
{
public:

    enum StdType	{
			    Anis, Area, Class, Comp, Den, Dist, ElaRa, ElPot,
			    GR, Imp, Perm, Pres, PresGrad, PresWt, Res, Son,
			    Temp, Time, Vel, Volum, Vol, Other
			};
			mDeclareEnumUtils(StdType)
    static StdType	surveyZType();

			PropertyRef( const char* nm, StdType t=Other )
			    : NamedObject(nm)
			    , stdtype_(t)		{}
			PropertyRef( const PropertyRef& pr )
			    : NamedObject(pr.name())	{ *this = pr; }
    virtual		~PropertyRef();
    PropertyRef&	operator =(const PropertyRef&);
    inline bool		operator ==( const PropertyRef& pr ) const
			{ return name() == pr.name(); }
    inline bool		operator !=( const PropertyRef& pr ) const
			{ return name() != pr.name(); }
    bool		isKnownAs(const char*) const;

    inline StdType	stdType() const			{ return stdtype_; }
    inline bool		hasType( StdType t ) const
			{ return stdtype_ == t; }
    inline bool		isCompatibleWith( const PropertyRef& pr ) const
			{ return hasType(pr.stdType()); }
    inline void		setStdType( StdType t ) { stdtype_ = t; }

    static const PropertyRef& undef();

protected:

    StdType		stdtype_;

};


