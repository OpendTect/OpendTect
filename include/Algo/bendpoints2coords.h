#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2009
________________________________________________________________________

*/

#include "algomod.h"
#include "coord.h"
#include "typeset.h"
#include <od_iosfwd.h>


/*!
\brief Inter/Extra-polate bendpoints to get the coordinates.

  Input is a set of coordinates and optionally ID numbers (if you don't pass
  IDs they will be generated 0, 1, ....). Then you can give an ID (usually in
  between the bend point IDs) and get the interpolated coordinate.

  The bend points will be sorted on ID.
*/

mExpClass(Algo) BendPoints2Coords
{
public:
    			BendPoints2Coords(const TypeSet<Coord>&,
					  const int* ids=0);
			BendPoints2Coords(od_istream&); //!< 'table' file

    const TypeSet<Coord>& getCoords() const	{ return coords_; }
    const TypeSet<int>&	getIDs() const		{ return nrs_; }

    Coord		coordAt(float) const;

    void		readFrom(od_istream&);

protected:

    TypeSet<int>	nrs_;
    TypeSet<Coord>	coords_;

    void		init(const TypeSet<Coord>&,const int*);
    void		getIndexes(float,Interval<int>&) const;
};

