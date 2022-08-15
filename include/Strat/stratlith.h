#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
________________________________________________________________________


-*/

#include "stratmod.h"

#include "color.h"
#include "integerid.h"
#include "namedobj.h"
#include "manobjectset.h"

class BufferStringSet;


namespace Strat
{

mExpClass(Strat) LithologyID : public IntegerID<od_int32>
{
public:

    using IntegerID::IntegerID;
    static inline LithologyID	udf()	  { return LithologyID(); }
    static inline LithologyID	unsetID() { return LithologyID(-2); }
};


/*!\brief a name and an ID.

  The reason we don't manage the ID is that user may have their own IDs. For
  example, it may correspond with a lithology log value.

  LithologyID: unsetID() -> id is unset. Should call getFreeID to get a valid ID
  LithologyID: isValid: Lithology is defined and has a unique ID

*/

mExpClass(Strat) Lithology : public ::NamedObject
{
public:

			Lithology(const LithologyID&,const char* nm,
				  bool por=false);
			Lithology( const Lithology& l )
			    : id_(l.id_)	{ *this = l; }
    Lithology&		operator =(const Lithology&);
    bool		operator ==( const Lithology& l ) const
						{ return l.id_ == id_; }

    bool		isUdf() const		{ return this == &undef(); }

    LithologyID		id() const		{ return id_; }
    bool&		porous()		{ return porous_; }
    bool		porous() const		{ return porous_; }
    OD::Color&		color()			{ return color_; }
    const OD::Color&	color() const		{ return color_; }

    static const Lithology& undef();

protected:

    const LithologyID	id_;
    bool		porous_;
    OD::Color		color_;

    friend class	LithologySet;

public:

			Lithology(const char*); // from string in file
    void		fill(BufferString&) const;

};


mExpClass(Strat) LithologySet : public CallBacker
{
public:
			LithologySet()
			    : anyChange(this)	{}

    int			size() const		{ return lths_.size(); }
    bool		isEmpty() const		{ return lths_.isEmpty(); }
    Lithology&		getLith( int i )	{ return *lths_[i]; }
    const Lithology&	getLith( int i ) const	{ return *lths_[i]; }

    int			indexOf(const char* nm) const;
    bool		isPresent(const char* nm) const;
    int			indexOf(const LithologyID&) const;
    bool		isPresent(const LithologyID&) const;

    Lithology*		get(const char* nm);
    const Lithology*	get(const char* nm) const;
    Lithology*		get(const LithologyID&);
    const Lithology*	get(const LithologyID&) const;

    enum PorSel		{ OnlyPorous, NotPorous, AllPor };
    void		getNames(BufferStringSet&,PorSel ps=AllPor) const;

    LithologyID		getFreeID() const;
    static LithologyID	getInitialID();

    void		reportAnyChange()		{ anyChange.trigger(); }
    Notifier<LithologySet> anyChange;

protected:

    ManagedObjectSet<Lithology>	lths_;

    int			idxOf(const char*,const LithologyID&) const;
    Lithology*		gtLith(const char* nm,const LithologyID&) const;

public:

    void			setEmpty()		{ lths_.erase(); }
    const char*			add(Lithology*);
				//!< returns err msg, or null on success

    const ObjectSet<Lithology>&	lithologies() const	{ return lths_; }
    ObjectSet<Lithology>&	lithologies()		{ return lths_; }

};


} // namespace Strat

