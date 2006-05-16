#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		May 2001
 Contents:	PickSet base classes
 RCS:		$Id: pickset.h,v 1.18 2006-05-16 16:28:22 cvsbert Exp $
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
			Location( double x=0, double y=0, double z=0 )
			: pos(x,y,z), text(0)			{}
			Location( const Coord& c, float f=0 )
			: pos(c,f), text(0)			{}
			Location( const Coord3& c )
			: pos(c), text(0)			{}
			Location( const Coord3& c, const Coord3& d )
			: pos(c), dir(d), text(0)		{}
			Location( const Coord3& c, const Sphere& d )
			: pos(c), dir(d), text(0)		{}
			Location( const Location& pl )
			: text(0)				{ *this = pl; }

			~Location();

    inline bool		operator ==( const Location& pl ) const
			{ return pos == pl.pos && dir == pl.dir; }
    inline bool		operator !=( const Location& pl ) const
			{ return !(*this == pl); }
    void		operator =(const Location&);

    bool		fromString(const char*,bool doxy=true);
    void		toString(char*);

    Coord3		pos;
    Sphere		dir; //!< Optional direction at location
    BufferString*	text; //!<Optional text at location

    void		setText(const char* key,const char* txt);

    inline bool		hasDir() const
    			{ return !mIsZero(dir.radius,mDefEps)
			      || !mIsZero(dir.theta,mDefEps)
			      || !mIsZero(dir.phi,mDefEps); }
	
    void		getKey(const char* key,BufferString&) const;
};


/*!\brief Set of picks with something in common */

class Set : public UserIDObject
	  , public TypeSet<Location>
{
public:

			Set( const char* nm=0 )
			    : UserIDObject(nm)
			    , color_(Color::NoColor)
			    , pixsize_(3)		{}
			Set( const Set& s )
			{ *this = s; }
    Set&		operator =( const Set& s )
			{
			    if ( &s == this ) return *this;
			    copy( s );
			    setName( s.name() );
			    color_ = s.color_; pixsize_ = s.pixsize_;
			    return *this;
			}


    Color		color_;		//!< Display color
    int			pixsize_;	//!< Display size in pixels
};


/*!\brief Utility to manage pick set lifecycles.
          Also supports change notifications.
 
 You can create your own set manager for your own special pick sets.
 There is a OD-wide Mgr() available which is supposed to hold all 'plain'
 picksets loaded in the OD-tree.

 A new special-purpose manager is created by passing your own name to the
 static getMgr() method.
 
 */

class SetMgr : public UserIDObject
{
public:

    int			size() const		{ return pss_.size(); }
    Set&		get( int idx )		{ return *pss_[idx]; }
    const Set&		get( int idx ) const	{ return *pss_[idx]; }
    const MultiID&	id( int idx ) const	{ return *ids_[idx]; }

    bool		isLoaded( const MultiID& i ) const { return find(i); }
    bool		isLoaded( const Set& s ) const { return find(s); }
    Set&		get( const MultiID& i )		{ return *find(i); }
    const Set&		get( const MultiID& i ) const	{ return *find(i); }
    const MultiID&	get( const Set& s ) const	{ return *find(s); }

    void		set(const MultiID&,Set*);
    			//!< add, replace or remove (pass null Set ptr).
    			//!< Set is already or becomes mine
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
    void		reportChange( const Set* s )
			{ setChanged.trigger( const_cast<Set*>(s) ); }
			//!< report a bunch of changes to a set

    Notifier<SetMgr> locationChanged;	//!< Passes ChangeData*
    Notifier<SetMgr> setToBeRemoved;	//!< Passes Set*
    Notifier<SetMgr> setAdded;		//!< passes Set*
    Notifier<SetMgr> setChanged;	//!< passes Set*

    static SetMgr&	getMgr(const char*);

    			SetMgr( const char* nm )
			: UserIDObject(nm)
			, locationChanged(this), setToBeRemoved(this)
			, setAdded(this), setChanged(this)	{}
			//!< creates an unmanaged SetMgr
			//!< Normally you don't want that, use getMgr() instead

protected:

    ObjectSet<Set>	pss_;
    ObjectSet<MultiID>	ids_;

    void		add(const MultiID&,Set*);
    Set*		find(const MultiID&) const;
    MultiID*		find(const Set&) const;
};

inline SetMgr& Mgr()
{
    return SetMgr::getMgr(0);
}


}; // namespace Pick


#endif
