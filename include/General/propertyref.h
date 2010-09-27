#ifndef propertyref_h
#define propertyref_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
 RCS:		$Id: propertyref.h,v 1.2 2010-09-27 10:00:11 cvsbert Exp $
________________________________________________________________________


-*/

#include "enums.h"
#include "namedobj.h"
#include "bufstringset.h"
#include "repos.h"

class ascistream;
class ascostream;


/*!\brief Ref Data for a (usually petrophysical) property.

We prepare for many variants of the name as is not uncommon in practice
(Density, Den, Rho, RhoB, ... you know the drill). The names will be unique
- case insensitive, in the Set. Hence, identity is established case insensitive.
Aliases are matched with a GlobExpr, so you can add with wildcards and the like.

 */

mClass PropertyRef : public NamedObject
{
public:

    enum StdType	{
			    Other, Time, Dist, Por, Perm, GR, Temp, Pres,
			    Den, Vel, Son, AI, Sat, ElPot, Res, PR, Comp
			};
			DeclareEnumUtils(StdType)
    static StdType	surveyZType();
    inline static bool	isHCAffected( StdType t )	{ return t >= Den; }

			PropertyRef( const char* nm, StdType t=Other )
			: NamedObject(nm)
			, stdtype_(t)			{}
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

    inline bool		isHCAffected() const
					{ return isHCAffected(stdtype_); }
    inline BufferStringSet&		aliases()	{ return aliases_; }
    inline const BufferStringSet&	aliases() const	{ return aliases_; }

    static const PropertyRef&		undef();

protected:

    StdType		stdtype_;
    BufferStringSet	aliases_;

    friend class	PropertyRefSet;
};


mClass PropertyRefSet : public ObjectSet<PropertyRef>
{
public:
    			PropertyRefSet()		{}
			~PropertyRefSet()		{ deepErase(*this); }

     inline bool	isPresent( const char* nm ) const
     			{ return indexOf(nm) >= 0; }
     int		indexOf(const char*) const;
     inline PropertyRef* get( const char* nm )		{ return gt(nm); }
     inline const PropertyRef* get( const char* nm ) const { return gt(nm); }

     int		add(PropertyRef*);
			//!< refuses if another one isKnownAs. If not added,
     			//!< clean up the mess yourself (i.e. delete it)
     virtual PropertyRefSet& operator +=( PropertyRef* pr )
     			{ add(pr); return *this; }

     bool		save(Repos::Source) const;

protected:

     PropertyRef*	gt(const char*) const;

public:

     void		readFrom(ascistream&);
     bool		writeTo(ascostream&) const;

};

const PropertyRefSet& PROPS();
inline PropertyRefSet& ePROPS() { return const_cast<PropertyRefSet&>(PROPS()); }


#endif
