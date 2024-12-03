#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "callback.h"


mExpClass(Geometry) Geom2DInit final : public CallBacker
{
public:
			~Geom2DInit();

    static Geom2DInit&	getInstance();
    void		start();

private:
			Geom2DInit();

    void		readGeomCB(CallBacker*);
    void		computeBendpointsCB(CallBacker*);
    void		computeIntersectionsCB(CallBacker*);

    void		closeQueueCB(CallBacker*);
    int			queueid_;
};
