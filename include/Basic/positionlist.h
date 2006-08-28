#ifndef positionlist_h
#define positionlist_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Jan 2003
 RCS:           $Id: positionlist.h,v 1.3 2006-08-28 09:33:04 cvskris Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"


class Coord3;
/*! Basic interface to a list of coordinates and management functions. */

class CoordList
{ mRefCountImplNoDestructor(CoordList);
public:
    virtual int		add(const Coord3&)				= 0;
    virtual Coord3	get(int) const					= 0;
    virtual void	set(int,const Coord3&)				= 0;
    virtual void	remove(int)					= 0;
};


#endif
