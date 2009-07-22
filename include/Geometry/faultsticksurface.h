#ifndef faultsticksurface_h
#define faultsticksurface_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl / J.C. Glas
Date:          September 2007
RCS:           $Id: faultsticksurface.h,v 1.14 2009-07-22 16:01:16 cvsbert Exp $
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
