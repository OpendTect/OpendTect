#ifndef faultsticksurface_h
#define faultsticksurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl / J.C. Glas
Date:          September 2007
RCS:           $Id: faultsticksurface.h,v 1.12 2008-11-18 13:28:53 cvsjaap Exp $
________________________________________________________________________

-*/

#include "faultstickset.h"

namespace Geometry
{

class FaultStickSurface : public FaultStickSet
{
public:
    			FaultStickSurface();

    bool		insertStick(const Coord3& firstpos,
				    const Coord3& editnormal,int stick=0,
				    int firstcol=0);

    bool		areSticksVertical() const;

protected:

    bool			sticksvertical_;
};

};

#endif
