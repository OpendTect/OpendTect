#ifndef propertyref_h
#define propertyref_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
 RCS:		$Id: propertyref.h,v 1.1 2010-09-24 13:39:22 cvsbert Exp $
________________________________________________________________________


-*/

#include "enums.h"
#include "namedobj.h"
#include "objectset.h"
#include "repos.h"

class ascistream;
class ascostream;


/*!\brief Ref Data for a (usually petrophysical) property */

mClass PropertyRef : public NamedObject
{
public:

    enum StdType	{ Other, Time, Dist, Den, Vel, Son, AI, Por, Perm,
			  Sat, GR, ElPot, Res, PR, Comp, Temp, Pres };
			DeclareEnumUtils(StdType)
    static StdType	surveyZType();

			PropertyRef( const char* nm, StdType t=Other,
				     bool h=false )
			: NamedObject(nm)
			, stdtype_(t)
			, hcaff_(h)			{}

    bool		hcAffected() const		{ return hcaff_; }
    void		setHCAffected( bool yn )	{ hcaff_ = yn; }
    StdType		stdType() const			{ return stdtype_; }
    void		setStdType( StdType t ) 	{ stdtype_ = t; }

    const PropertyRef&	undef();

protected:

    StdType		stdtype_;
    bool		hcaff_;

    friend class	PropertyRefSet;
};


/* Set of PropertyRef's. Name is matched case indep. */

mClass PropertyRefSet : public ObjectSet<PropertyRef>
{
public:
    			PropertyRefSet()		{}
			~PropertyRefSet()		{ deepErase(*this); }

     PropertyRef*	get( const char* nm )		{ return gt(nm); }
     const PropertyRef*	get( const char* nm ) const	{ return gt(nm); }

     void		add(PropertyRef*); //!< if name exists, will remove old

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
