#ifndef property_h
#define property_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: property.h,v 1.3 2004-02-19 16:22:26 bert Exp $
________________________________________________________________________


-*/

#include "enums.h"
#include "uidobj.h"
#include "bufstringset.h"

/*!\brief Ref Data for a (usually petrophysical) property */

class PropertyRef : public ::UserIDObject
{
public:

    enum StdType	{ Other, Time, Dist, Den, Vel, Son, AI, Por, Perm,
			  Sat, GR, ElPot, Res, PR, Comp, Temp, Pres };
			DeclareEnumUtilsWithVar(StdType,stdtype)

			PropertyRef( const char* nm=0, StdType t=Other,
				     bool h=false )
			: UserIDObject(nm)
			, stdtype_(t)
			, hcaff_(h)			{}

    BufferStringSet&	specialUnitsOfMeasure()		{ return units_; }
    bool		hcAffected() const		{ return hcaff_; }
    void		setHCAffected( bool yn )	{ hcaff_ = yn; }

protected:

    BufferStringSet	units_;
    bool		hcaff_;

};


/*!\brief A (usually petrophysical) property of some object */

class Property
{
public:

    			Property(const PropertyRef* pr )
			: ref_(pr)		{}
    virtual		~Property()		{}

    virtual float	value() const		= 0;
    const PropertyRef*	ref() const		{ return ref_; }

    virtual bool	dependsOn(const Property*) const { return false; }

protected:

    const PropertyRef*	ref_;

};


/*\brief The repository of all PropertyRefs in OpendTect

  The singleton instance can be accessed through the global PrRR() function.

  */

class PropertyRefRepository
{
public:

    const PropertyRef* get(const char* nm) const;
    			//!< Will try names first, then symbols, otherwise null

    const ObjectSet<const PropertyRef>& all() const	{ return entries; }

    bool		add(const PropertyRef&);
    			//!< returns whether already present
    			//!< Note that add is temporary for this run of OD

private:

    			PropertyRefRepository();

    ObjectSet<const PropertyRef> entries;

    void		addFromFile(const char*);

    friend PropertyRefRepository& PrRR();

};


#endif
