#ifndef positionlist_h
#define positionlist_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Jan 2003
 RCS:           $Id: positionlist.h,v 1.1 2006-05-03 14:46:29 cvskris Exp $
________________________________________________________________________

-*/

#include "refcount.h"


class Coord3;
/*! Basic interface to a list of coordinates and management functions. */

class CoordList
{ mRefCountImplNoDestructor(CoordList);
public:
    			CoordList() { mRefCountConstructor; }
    virtual int		add(const Coord3&)				= 0;
    virtual Coord3&	get(int) const					= 0;
    virtual bool	set(int,const Coord3&) const			= 0;
    virtual int		remove(int)					= 0;
};


#endif
