#ifndef survgeometry_h
#define survgeometry_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman K Singh
 Date:          July 2009
 RCS:           $Id: survgeometry.h,v 1.1 2009/07/23 05:10:26 cvsraman Exp $
________________________________________________________________________

-*/

#include "position.h"

mClass SurveyGeometry
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

#endif
