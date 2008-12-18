#ifndef positionlist_h
#define positionlist_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Jan 2003
 RCS:           $Id: positionlist.h,v 1.7 2008-12-18 05:23:26 cvsranojay Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"


class Coord3;
/*! Basic interface to an list of Coord3 where each coord has a unique id. */

mClass Coord3List
{ mRefCountImplNoDestructor(Coord3List);
public:
    virtual int		nextID(int previd) const			= 0;
    			//!<If previd == -1, first id is returned.
			//!<If -1 is returned, no more id's are available.
    virtual int		add(const Coord3&)				= 0;
    			//!<Return new id, or -1 if unsuccessful
    virtual Coord3	get(int id) const				= 0;
    virtual void	set(int id,const Coord3&)			= 0;
    virtual void	remove(int id)					= 0;
};

/*! Basic interface to an list of Coord where each coord has a unique id. */

mClass Coord2List
{ mRefCountImplNoDestructor(Coord2List);
public:
    virtual int		nextID(int previd) const			= 0;
    			//!<If previd == -1, first id is returned.
			//!<If -1 is returned, no more id's are available.
    virtual Coord	get(int id) const				= 0;
    virtual void	set(int id,const Coord&)			= 0;
    virtual int		add(const Coord&)				= 0;
    			//!<Return new id, or -1 if unsuccessful
    virtual void	remove(int id)					= 0;
};


mClass Coord2ListImpl : public Coord2List
{ 
public:			
    			Coord2ListImpl();
    int			nextID(int previd) const;
    Coord		get(int id) const;
    void		set(int id,const Coord&);	
    int			add(const Coord&); 		
    void		remove(int id);
    int			getSize() const 	{ return points_.size(); }

protected:

    TypeSet<int>	removedids_;
    TypeSet<Coord> 	points_;
};

#endif
