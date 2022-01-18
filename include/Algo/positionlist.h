#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/

#include "algomod.h"
#include "refcount.h"

#include "position.h"
#include "typeset.h"

/*!
\brief Base class for vertex attribute list.
*/

mExpClass(Algo) FloatVertexAttribList : public ReferencedObject
{
public:

    virtual int		size() const				= 0;
    virtual bool	setSize(int,bool cpdata)		= 0;

    virtual void	setCoord(int,const float*)		= 0;
    virtual void	getCoord(int,float*) const		= 0;

    virtual void	setNormal(int,const float*)		= 0;
    virtual void	getNormal(int,float*) const		= 0;

    virtual void	setTCoord(int,const float*)		= 0;
    virtual void	getTCoord(int,float*) const		= 0;
};


/*!\brief Interface for a list of Coords with automatically maintained IDs. */

mExpClass(Algo) Coord2List : public ReferencedObject
{
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
    virtual void	remove(const TypeSet<int>&)			= 0;
    virtual int		size() const					= 0;

    Coord		center() const;
};


class Coord3;

/*!\brief Interface for a list of Coord3 with automatically maintained IDs. */

mExpClass(Algo) Coord3List : public ReferencedObject
{
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
    virtual void	remove(const TypeSet<int>&)			= 0;
			//!<May contain duplicates.
    virtual int		size() const					= 0;

    Coord3		center() const;
};


/*!
\brief A list of Coord where each coord has a unique id.
*/

mExpClass(Algo) Coord2ListImpl : public Coord2List
{
public:
			Coord2ListImpl();

    virtual int		nextID(int previd) const;
    virtual Coord	get(int id) const;
    virtual void	set(int id,const Coord&);
    virtual int		add(const Coord&);
    virtual void	remove(int id);
    virtual void	remove(const TypeSet<int>&);
    virtual void	addValue(int id,const Coord&);
    virtual int		size() const
			{ return points_.size() - removedids_.size(); }

protected:

    TypeSet<int>	removedids_;
    TypeSet<Coord>	points_;
};


/*!
\brief A list of Coord3 where each coord has a unique id.
*/

mExpClass(Algo) Coord3ListImpl : public Coord3List
{
public:
			Coord3ListImpl();

    virtual int		nextID(int previd) const;
    virtual Coord3	get(int id) const;
    virtual void	set(int id,const Coord3&);
    virtual int		add(const Coord3&);
    virtual bool	isDefined(int) const;
    virtual void	remove(int id);
    virtual void	addValue(int id,const Coord3&);
    virtual void	remove(const TypeSet<int>&);
    virtual int		size() const
			{ return coords_.size() - removedids_.size(); }

protected:

    TypeSet<int>	removedids_;
    TypeSet<Coord3>	coords_;
};
