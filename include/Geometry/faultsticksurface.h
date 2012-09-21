#ifndef faultsticksurface_h
#define faultsticksurface_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        K. Tingdahl / J.C. Glas
Date:          September 2007
RCS:           $Id$
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "faultstickset.h"

namespace Geometry
{

mClass(Geometry) FaultStickSurface : public FaultStickSet
{
public:
    			FaultStickSurface();

    bool		insertStick(const Coord3& firstpos,
				    const Coord3& editnormal,int stick=0,
				    int firstcol=0);

    bool		areSticksVertical() const;
    void		setSticksVertical(bool yn)	{ sticksvertical_=yn; }

protected:

    bool			sticksvertical_;
};

};

#endif

