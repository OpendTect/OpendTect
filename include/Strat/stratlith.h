#ifndef stratlith_h
#define stratlith_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratlith.h,v 1.12 2012-08-03 13:00:43 cvskris Exp $
________________________________________________________________________


-*/

#include "stratmod.h"
#include "namedobj.h"
#include "manobjectset.h"
#include "color.h"
class BufferStringSet;


namespace Strat
{

/*!\brief a name and an ID.
  
  The reason we don't manage the ID is that user may have their own IDs. For
  example, it may correspond with a lithology log value.

*/

mClass(Strat) Lithology : public ::NamedObject
{
public:

    typedef int		ID;

			Lithology(ID id,const char* nm,bool por=false);
			Lithology( const Lithology& l )
			    : id_(l.id_)	{ *this = l; }
    Lithology&		operator =(const Lithology&);
    bool		operator ==( const Lithology& l ) const
						{ return l.id_ == id_; }

    bool		isUdf() const		{ return this == &undef(); }
    static bool		isUdf( ID id )		{ return id == undef().id_; }

    ID			id() const		{ return id_; }
    bool&		porous()		{ return porous_; }
    bool		porous() const		{ return porous_; }
    Color&		color()			{ return color_; }
    const Color&	color() const		{ return color_; }

    static const Lithology& undef();

protected:

    const ID		id_;
    bool		porous_;
    Color		color_;

    friend class	LithologySet;

public:

			Lithology(const char*); // from string in file
    void		fill(BufferString&) const;

};


mClass(Strat) LithologySet : public CallBacker
{
public:
    			LithologySet()
			    : lths_(false), anyChange(this)	{}

    int			size() const		{ return lths_.size(); }
    bool		isEmpty() const		{ return lths_.isEmpty(); }
    Lithology&		getLith( int i )	{ return *lths_[i]; }
    const Lithology&	getLith( int i ) const	{ return *lths_[i]; }

    int		indexOf( const char* nm ) const		{ return idxOf(nm,-2);}
    bool	isPresent(const char* nm) const 	{ return gtLith(nm,-2);}
    int		indexOf( Lithology::ID id ) const	{ return idxOf(0,id);}
    bool	isPresent( Lithology::ID id ) const	{ return gtLith(0,id);}

    Lithology*		get( const char* nm )		{ return gtLith(nm,-2);}
    const Lithology*	get( const char* nm ) const	{ return gtLith(nm,-2);}
    Lithology*		get( Lithology::ID id )		{ return gtLith(0,id); }
    const Lithology*	get( Lithology::ID id ) const	{ return gtLith(0,id); }

    enum PorSel		{ OnlyPorous, NotPorous, AllPor };
    void		getNames(BufferStringSet&,PorSel ps=AllPor) const;

    void		reportAnyChange()		{ anyChange.trigger(); }
    Notifier<LithologySet> anyChange;

protected:

    ManagedObjectSet<Lithology>	lths_;

    int			idxOf(const char*,Lithology::ID) const;
    Lithology*		gtLith( const char* nm, Lithology::ID id ) const
			{ const int idx = idxOf(nm,id); return idx < 0 ? 0
				    : const_cast<Lithology*>(lths_[idx]); }

public:

    void			setEmpty()		{ lths_.erase(); }
    const char*			add(Lithology*);
    				//!< returns err msg, or null on success

    const ObjectSet<Lithology>&	lithologies() const	{ return lths_; }
    ObjectSet<Lithology>&	lithologies()	 	{ return lths_; }

};


}; // namespace Strat

#endif

