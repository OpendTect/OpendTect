#ifndef parametriccurve_h
#define parametriccurve_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: parametriccurve.h,v 1.2 2005-02-17 10:25:47 cvskris Exp $
________________________________________________________________________

-*/

#include "geomelement.h"

namespace Geometry
{
class ParametricCurve : public Element
{
public:
    virtual Coord3 	computePosition( float ) const	= 0;
    virtual Coord3 	computeDirection( float ) const	= 0;

    virtual bool	isCircular() const { return false; }
    void		getPosIDs( TypeSet<GeomPosID>& ) const;
    virtual bool	insertPosition( const GeomPosID,const Coord3& )	= 0;
    virtual StepInterval<int>	parameterRange() const			= 0;
};

};

#endif
