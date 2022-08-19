#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "coord.h"


mExpClass(Geometry) SurveyGeometry
{
public:

    static Coord	getEdgePoint(const Coord& from, const Coord& to);
    			/*!< Calculates where the line (or extension of it)
			     through the coords exits the survey box.
			     if !hasEdgePoint() returns the nearest corner. */
    static bool		hasEdgePoint(const Coord&,const Coord&);
    			/*!< Calculates whether the line between the coords
			     visits the survey box. */

};
