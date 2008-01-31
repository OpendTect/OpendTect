#ifndef positionlist_h
#define positionlist_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Jan 2003
 RCS:           $Id: positionlist.h,v 1.5 2008-01-31 21:21:03 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"


class Coord3;
/*! Basic interface to an list of Coord3 where each coord has a unique id. */

class Coord3List
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

class Coord2List
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


class TypeSetCoord2: public Coord2List
{ 
public:

    int			nextID(int previd) const 
    			{
			   if ( previd == points_.size()-1 ) 
			       return -1;
			   else 
			       return indices_[previd+1]; 
			}

    Coord		get(int id) const 		
    			{ 
			    if ( id<0 || id>=points_.size() )
				return Coord( mUdf(double), mUdf(double) );
			    else
    				return points_[id]; 
			}

    void		set(int id,const Coord& co)	
    			{ 
			    if ( id<0 || id>=points_.size() )
				return;

			    points_[id] = co; 
			}

    int			add(const Coord& co) 		
    			{ 
			    points_ += co; 
			    indices_ += points_.size()-1;
			    return points_.size()-1;
			}

    void		remove(int id)
			{
			    indices_ -= id;
			    points_ -= points_[id];
			}

protected:

    TypeSet<int> 	indices_;
    TypeSet<Coord> 	points_;
};

#endif
