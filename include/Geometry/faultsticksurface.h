#ifndef faultsticksurface_h
#define faultsticksurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl / J.C. Glas
Date:          September 2007
RCS:           $Id: faultsticksurface.h,v 1.13 2008-12-25 11:55:38 cvsranojay Exp $
________________________________________________________________________

-*/

#include "faultstickset.h"

namespace Geometry
{

mClass FaultStickSurface : public FaultStickSet
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
