#ifndef bendpoints2coords_h
#define bendpoints2coords_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2009
 RCS:		$Id: bendpoints2coords.h,v 1.1 2009/12/07 14:02:21 cvsbert Exp $
________________________________________________________________________

*/

#include "position.h"
#include "typeset.h"
#include <iosfwd>


/*! Inter/Extra-polate bendpoints to get the coordinates.

  Input is a set of coordinates and optionally ID numbers (if you don't pass
  IDs they will be generated 0, 1, ....). Then you can give an ID (usually in
  between the bend point IDs) and get the interpolated coordinate.

  The bend points will be sorted on ID.

 */

mClass BendPoints2Coords
{
public:

    			BendPoints2Coords(const TypeSet<Coord>&,
					  const int* ids=0);
    			BendPoints2Coords(std::istream&); //!< 'table' file

    const TypeSet<Coord>& getCoords() const	{ return coords_; }
    const TypeSet<int>&	getIDs() const		{ return nrs_; }

    Coord		coordAt(float) const;

    void		readFrom(std::istream&);

protected:

    TypeSet<int>	nrs_;
    TypeSet<Coord>	coords_;

    void		init(const TypeSet<Coord>&,const int*);
    void		getIndexes(float,Interval<int>&) const;
};


#endif
