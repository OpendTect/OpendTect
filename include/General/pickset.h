#ifndef pickset_h
#define pickset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		May 2001
 Contents:	PickSet base classes
 RCS:		$Id: pickset.h,v 1.12 2005-10-27 15:04:19 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "sets.h"
#include "trigonometry.h"
#include "color.h"


/*!\brief Pick location in space */

class PickLocation
{
public:
			PickLocation( double x=0, double y=0, float f=0 )
			: pos(x,y), z(f), text(0)			{}
			PickLocation( const Coord& c, float f=0 )
			: pos(c), z(f), text(0)				{}
			PickLocation( const Coord3& c )
			: pos(c.x,c.y), z(c.z), text(0)			{}
			PickLocation( const Coord3& c, const Coord3& d )
			: pos(c.x,c.y), z(c.z), dir(d), text(0)		{}
			PickLocation( const Coord3& c, const Sphere& d )
			: pos(c.x,c.y), z(c.z), dir(d), text(0)		{}

    inline bool		operator ==( const PickLocation& pl ) const
			{ return pos == pl.pos && mIsEqual(z,pl.z,mDefEps); }
    inline bool		operator !=( const PickLocation& pl ) const
			{ return !(*this == pl); }

    bool		fromString(const char*,bool doxy=true);
    void		toString(char*);

    Coord		pos;
    float		z;

    Sphere		dir; //!< Optional direction at location
    BufferString*	text; //!<Optional text at location

    inline bool		hasDir() const
    			{ return !mIsZero(dir.radius,mDefEps)
			      || !mIsZero(dir.theta,mDefEps)
			      || !mIsZero(dir.phi,mDefEps); }
	
    void		getKey(const char* key,BufferString&) const;
};


/*!\brief Group of picks with something in common */

class PickSet : public UserIDObject
	      , public TypeSet<PickLocation>
{
public:

			PickSet(const char* nm);

    Color		color;

};


/*!\brief Set of Pick Groups */

class PickSetGroup : public UserIDObject
{
public:

			PickSetGroup( const char* nm=0 )
			: UserIDObject(nm)	{}
			~PickSetGroup()		{ clear(); }

    int			nrSets() const		{ return sets.size(); }
    PickSet*		get( int nr )		{ return sets[nr]; }
    const PickSet*	get( int nr ) const	{ return sets[nr]; }
    void		add(PickSet*&);
			//!< PickSet becomes mine. Will merge if necessary.
			//!< So PickSet may be deleted (will be set to null)
    void		remove( int idx )
			{ delete sets[idx]; sets.remove(idx); }
    void		clear()			{ deepErase(sets); }

protected:

    ObjectSet<PickSet> sets;

};


#endif
