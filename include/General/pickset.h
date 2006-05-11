#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		May 2001
 Contents:	PickSet base classes
 RCS:		$Id: pickset.h,v 1.17 2006-05-11 12:56:24 cvsbert Exp $
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


/*!\brief Utility to manage pick set group lifecycles.
          Also supports change notifications.
 
 You can create your own set group manager for your own special pick sets.
 There is a OD-wide SGMgr() available which is supposed to hold all 'plain'
 picksets loaded in the OD-tree.

 A new special-purpose manager is created by passing your own name to the
 static getMgr() method.
 
 */

class SetGroupMgr : public UserIDObject
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

    struct ChangeData : public CallBacker
    {
	enum Ev		{ Added, Changed, ToBeRemoved };

			ChangeData( Ev e, const Set* s, const Location* l )
			    : ev_(e), set_(s), loc_(l)	{}

	Ev		ev_;
	const Set*	set_;
	const Location*	loc_;
    };
    
    void		reportChange( ChangeData cd )
			{ locationChanged.trigger( &cd ); }
    void		reportChange( const SetGroup* s )
			{ groupChanged.trigger( const_cast<SetGroup*>(s) ); }
			//!< report a bunch of changes to a set

    Notifier<SetGroupMgr> locationChanged;	//!< Passes ChangeData*
    Notifier<SetGroupMgr> groupToBeRemoved;	//!< Passes Group*
    Notifier<SetGroupMgr> groupAdded;		//!< passes Group*
    Notifier<SetGroupMgr> groupChanged;		//!< passes Group*

    static SetGroupMgr&	getMgr(const char*);

    			SetGroupMgr( const char* nm )
			: UserIDObject(nm)
			, locationChanged(this), groupToBeRemoved(this)
			, groupAdded(this), groupChanged(this)	{}
			//!< creates an unmanaged SetGroupMgr
			//!< Normally you don't want that, use getMgr() instead

protected:

    ObjectSet<SetGroup>	psgs_;
    ObjectSet<MultiID>	ids_;

    void		add(const MultiID&,SetGroup*);
    SetGroup*		find(const MultiID&) const;
    MultiID*		find(const SetGroup&) const;
};

inline SetGroupMgr& SGMgr()
{
    return SetGroupMgr::getMgr(0);
}


}; // namespace Pick


#endif
