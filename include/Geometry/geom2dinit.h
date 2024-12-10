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

    static void		Start();

private:
			Geom2DInit();

    void		start();
    void		readGeomCB(CallBacker*);
    void		geomReadyCB(CallBacker*);
    void		bendPointsReadyCB(CallBacker*);
    void		computeBendpointsCB(CallBacker*);
    void		computeIntersectionsCB(CallBacker*);
    void		rootDirChangedCB(CallBacker*);

    void		closeQueueCB(CallBacker*);
    bool		continue_	= true;
    int			queueid_;
    Threads::Lock	lock_;

    static Geom2DInit&	getInstance();
};
