#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		May 2001
 Contents:	PickSet base classes
 RCS:		$Id: pickset.h,v 1.16 2006-05-08 16:50:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "sets.h"
#include "trigonometry.h"
#include "color.h"
class MultiID;


namespace Pick
{

/*!\brief Pick location in space */

class Location
{
public:
			Location( double x=0, double y=0, float f=0 )
			: pos(x,y), z(f), text(0)			{}
			Location( const Coord& c, float f=0 )
			: pos(c), z(f), text(0)				{}
			Location( const Coord3& c )
			: pos(c.x,c.y), z(c.z), text(0)			{}
			Location( const Coord3& c, const Coord3& d )
			: pos(c.x,c.y), z(c.z), dir(d), text(0)		{}
			Location( const Coord3& c, const Sphere& d )
			: pos(c.x,c.y), z(c.z), dir(d), text(0)		{}
			Location( const Location& pl )
			: text(0)
			{ *this = pl; }

			~Location();

    inline bool		operator ==( const Location& pl ) const
			{ return pos == pl.pos && mIsEqual(z,pl.z,mDefEps); }
    inline bool		operator !=( const Location& pl ) const
			{ return !(*this == pl); }
    void		operator =(const Location&);

    bool		fromString(const char*,bool doxy=true);
    void		toString(char*);

    Coord		pos;
    float		z;

    Sphere		dir; //!< Optional direction at location
    BufferString*	text; //!<Optional text at location
    void		setText(const char* key,const char* txt);

    inline bool		hasDir() const
    			{ return !mIsZero(dir.radius,mDefEps)
			      || !mIsZero(dir.theta,mDefEps)
			      || !mIsZero(dir.phi,mDefEps); }
	
    void		getKey(const char* key,BufferString&) const;
};


/*!\brief Group of picks with something in common */

class Set : public UserIDObject
	  , public TypeSet<Location>
{
public:

			Set(const char* nm);

    Color		color_;		//!< Display color
    int			pixsize_;	//!< Display size in pixels
};


/*!\brief Set of Pick Groups */

class SetGroup : public UserIDObject
{
public:

			SetGroup( const char* nm=0 )
			: UserIDObject(nm)	{}
			~SetGroup()		{ clear(); }

    int			nrSets() const		{ return sets.size(); }
    Set*		get( int nr )		{ return sets[nr]; }
    const Set*	get( int nr ) const	{ return sets[nr]; }
    void		add(Set*&);
			//!< Set becomes mine. Will merge if necessary.
			//!< So Set may be deleted (will be set to null)
    void		remove( int idx )
			{ delete sets[idx]; sets.remove(idx); }
    void		clear()			{ deepErase(sets); }

protected:

    ObjectSet<Set>	sets;

};


class SetGroupMgr : public CallBacker
{
public:

    int			size() const		{ return psgs_.size(); }
    SetGroup&		get( int idx )		{ return *psgs_[idx]; }
    const SetGroup&	get( int idx ) const	{ return *psgs_[idx]; }
    const MultiID&	id( int idx ) const	{ return *ids_[idx]; }

    bool		isLoaded( const MultiID& i ) const { return find(i); }
    bool		isLoaded( const SetGroup& s ) const { return find(s); }
    SetGroup&		get( const MultiID& i )		{ return *find(i); }
    const SetGroup&	get( const MultiID& i ) const	{ return *find(i); }
    const MultiID&	get( const SetGroup& s ) const	{ return *find(s); }

    void		set(const MultiID&,SetGroup*);
    			//!< add, replace or remove (pass null SetGroup ptr).
    			//!< SetGroup is already or becomes mine
    			//!< Note that replacement will trigger two callbacks

    Notifier<SetGroupMgr> itemToBeRemoved; //!< Passes doomed grp as CallBacker*
    Notifier<SetGroupMgr> itemAdded;       //!< passes new grp as CallBacker*

protected:
    			SetGroupMgr()
			    : itemAdded(this), itemToBeRemoved(this)	{}

    ObjectSet<SetGroup>	psgs_;
    ObjectSet<MultiID>	ids_;

    friend SetGroupMgr&	SGMgr();
    static SetGroupMgr*	theinst_;

    void		add(const MultiID&,SetGroup*);
    SetGroup*		find(const MultiID&) const;
    MultiID*		find(const SetGroup&) const;
};

SetGroupMgr& SGMgr();


}; // namespace


#endif
