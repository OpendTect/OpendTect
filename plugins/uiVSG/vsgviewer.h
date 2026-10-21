#pragma once
/*+
________________________________________________________________________

 Copyright: (C) 1995-2026 dGB Beheer B.V.
 License:   https://dgbes.com/licensing
________________________________________________________________________

 The frame timer for a VSG viewer embedded in Qt. Adapted from vsgQt
 0.5.0 (https://github.com/vsg-dev/vsgQt)
 */

#include "uivsgmod.h"
#include "commondefs.h"
#include "visvsgdata.h"

#ifdef min
# undef min
#endif
#ifdef max
# undef max
#endif

#include <vsg/app/Viewer.h>

#include <atomic>

#include <QTimer>


mClass(uiVSG) dGBVSGViewer : public vsg::Inherit<vsg::Viewer, dGBVSGViewer>
			   , public dGBVis::RedrawRequester
{
public:
		dGBVSGViewer(int msectimerinterval=0);
		~dGBVSGViewer();

    void	request() override;
    void	stop();
    void	render(double simulationtime
					=vsg::Viewer::UseTimeSinceStartPoint);

    bool	pollEvents(bool discardpreviousevents=true) override;

private:

    void	setInterval(int msectimerinterval);

    QTimer	timer_;
    std::atomic_uint	requests_	= 0;
    bool	continuousupdate_   = true;

};

EVSG_type_name(dGBVSGViewer);
