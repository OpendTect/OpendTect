#ifndef positionlist_h
#define positionlist_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "refcount.h"
#include "position.h"

/*!
\ingroup Basic
\brief Base class for vertex attribute list.
*/

mExpClass(Basic) FloatVertexAttribList
{ mRefCountImpl(FloatVertexAttribList)
public:
    
    
    virtual int		size() const				= 0;
    virtual bool	setSize(int,bool cpdata)		= 0;
    
    virtual void	setCoord(int,const float*)		= 0;
    virtual void	getCoord(int,float*) const		= 0;
    
    virtual void	setNormal(int,const float*)		= 0;
    virtual void	getNormal(int,float*) const		= 0;
    
    virtual void	setTCoord(int,const float*)		= 0;
    virtual void	getTCoord(int,float*) const		= 0;
    
protected:
};


class Coord3;

/*!
\ingroup Basic
\brief Use Coord3ListImpl instead.
*/

mExpClass(Basic) Coord3List
{ mRefCountImplNoDestructor(Coord3List);
public:
    virtual int		nextID(int previd) const			= 0;
    			//!<If previd == -1, first id is returned.
			//!<If -1 is returned, no more id's are available.
    virtual int		add(const Coord3&)				= 0;
    			//!<Return new id, or -1 if unsuccessful
    virtual Coord3	get(int id) const				= 0;
    virtual bool	isDefined(int id) const				= 0;
    virtual void	set(int id,const Coord3&)			= 0;
    virtual void	addValue(int id,const Coord3&)			= 0;
    			//!<Adds value to existing value at id
    virtual void	remove(int id)					= 0;
};


/*!
\ingroup Basic
\brief Use Coord2ListImpl instead.
*/

mExpClass(Basic) Coord2List
{ mRefCountImplNoDestructor(Coord2List);
public:
    virtual int		nextID(int previd) const			= 0;
    			//!<If previd == -1, first id is returned.
			//!<If -1 is returned, no more id's are available.
    virtual Coord	get(int id) const				= 0;
    virtual void	set(int id,const Coord&)			= 0;
    virtual int		add(const Coord&)				= 0;
    			//!<Return new id, or -1 if unsuccessful
    virtual void	addValue(int id,const Coord&)			= 0;
    			//!<Adds value to existing value at id
    virtual void	remove(int id)					= 0;
};


/*!
\ingroup Basic
\brief A list of Coord where each coord has a unique id.
*/

mExpClass(Basic) Coord2ListImpl : public Coord2List
{ 
public:			
    			Coord2ListImpl();
    int			nextID(int previd) const;
    Coord		get(int id) const;
    void		set(int id,const Coord&);	
    int			add(const Coord&); 		
    void		remove(int id);
    void		addValue(int id,const Coord&);
    int			getSize() const 	{ return points_.size(); }

protected:

    TypeSet<int>	removedids_;
    TypeSet<Coord> 	points_;
};


/*!
\ingroup Basic
\brief A list of Coord3 where each coord has a unique id. 
*/

mExpClass(Basic) Coord3ListImpl : public Coord3List
{
public:
    			Coord3ListImpl();
    int                 nextID(int previd) const;
    Coord3              get(int id) const;
    void                set(int id,const Coord3&);
    int                 add(const Coord3&);
    bool		isDefined(int) const;
    void                remove(int id);
    int                 getSize() const         { return coords_.size(); }
    void		addValue(int id,const Coord3&);

protected:

    TypeSet<int>        removedids_;
    TypeSet<Coord3>     coords_;
};


#endif

