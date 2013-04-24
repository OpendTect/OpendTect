#ifndef propertyref_h
#define propertyref_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
 RCS:		$Id$
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

class Property;
class ascistream;
class ascostream;


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
			    Anis, Class, Comp, Den, Dist, ElaRa, ElPot, GR,
			    Imp, Perm, Pres, PresWt, Res, Son, Temp, Time,
			    Vel, Volum, Other
			};
			DeclareEnumUtils(StdType)
    static StdType	surveyZType();

			PropertyRef( const char* nm, StdType t=Other )
			: NamedObject(nm)
			, stdtype_(t)			{}
			PropertyRef( const PropertyRef& pr )
			{ *this = pr; }
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
    inline void		setStdType( StdType t ) 	{ stdtype_ = t; }

    inline BufferStringSet& aliases()			{ return aliases_; }
    inline const BufferStringSet& aliases() const	{ return aliases_; }

    static const PropertyRef& undef();

    // Defaults for display
    mStruct(General) DispDefs
    {
			DispDefs()
			: color_(Color::Black())
			, defval_(0)
			, range_(mUdf(float),mUdf(float))	{}
			~DispDefs();

	Color		color_;
	Property*	defval_;
	Interval<float>	range_;		//!< Internal units
	BufferString	unit_;

	float		possibleValue() const;
    };

    DispDefs		disp_;

    static const PropertyRef& thickness();
    		//!< use this always. It has automatic defaults from SI()
    inline bool		isThickness() const	{ return this == &thickness(); }

protected:

    StdType		stdtype_;
    BufferStringSet	aliases_;

    friend class	PropertyRefSet;
    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

};


mExpClass(General) PropertyRefSet : public ObjectSet<PropertyRef>
{
public:

    			PropertyRefSet()		{}
    			PropertyRefSet( const PropertyRefSet& prs )
							{ *this = prs; }
			~PropertyRefSet()		{ deepErase(*this); }
    PropertyRefSet&	operator =(const PropertyRefSet&);

    inline bool		isPresent( const char* nm ) const
     			{ return indexOf(nm) >= 0; }
    int			indexOf(const char*) const;
    int			indexOf(PropertyRef::StdType,int occ=0) const;
    inline PropertyRef*	find( const char* nm )		{ return fnd(nm); }
    inline const PropertyRef* find( const char* nm ) const { return fnd(nm); }

    int			add(PropertyRef*);
			//!< refuses if another one isKnownAs. If not added,
     			//!< clean up the mess yourself (i.e. delete it)
    virtual PropertyRefSet& operator +=( PropertyRef* pr )
     			{ add(pr); return *this; }
    int			ensurePresent(PropertyRef::StdType,const char* nm1,
	    			      const char* nm2=0,const char* nm3=0);

    bool		save(Repos::Source) const;

    inline bool		isPresent( const PropertyRef* pr ) const
			{ return ObjectSet<PropertyRef>::isPresent(pr); }
    int			indexOf( const PropertyRef* pr ) const
			{ return ObjectSet<PropertyRef>::indexOf(pr); }

protected:

    PropertyRef*	fnd(const char*) const;

public:

    void		readFrom(ascistream&);
    bool		writeTo(ascostream&) const;

};

mGlobal(General) const PropertyRefSet& PROPS();
inline PropertyRefSet& ePROPS() { return const_cast<PropertyRefSet&>(PROPS()); }


mExpClass(General) PropertyRefSelection : public ObjectSet<const PropertyRef>
{
public:

    			PropertyRefSelection();
    bool		operator ==(const PropertyRefSelection&) const;

    int			indexOf(const char*) const;
    int			find(const char*) const; // also uses 'isKnownAs'

    inline bool		isPresent( const char* prnm ) const
			{ return indexOf( prnm ) >= 0; }
    inline int		indexOf( const PropertyRef* pr ) const
			{ return ObjectSet<const PropertyRef>::indexOf(pr); }
    inline bool		isPresent( const PropertyRef* pr ) const
			{ return ObjectSet<const PropertyRef>::isPresent(pr); }

    inline const PropertyRef* get( const char* nm ) const
			{ const int idx = indexOf(nm);
			  return idx < 0 ? 0 : (*this)[idx]; }

};


#endif


