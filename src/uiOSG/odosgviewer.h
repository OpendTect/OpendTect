#pragma once
/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		January 2022
________________________________________________________________________

-*/


#include <QObject>
#include <osgViewer/Viewer>

class QInputEvent;
class QKeyEvent;
class QMouseEvent;
class QTimerEvent;
class QWheelEvent;

/*
   This class is a slightly modified version of the OSGRenderer class
   available at: https://github.com/openscenegraph/osgQt
*/


class ODOSGViewer : public QObject, public osgViewer::Viewer
{
public:
				ODOSGViewer(QObject* parent);
				~ODOSGViewer();

    void			doInit();

    bool			checkNeedToDoFrame() override;
    void			frame(double simulationtime=USE_REFERENCE_TIME)
								override;
    bool			checkEvents() override;

    void			update();

protected:
    void			timerEvent(QTimerEvent*) override;

    float			windowscale_		= 1.f;

    int				timerid_		= 0;
    osg::Timer			lastframestarttime_;
    bool			applicationabouttoquit_	= false;
    bool			osgwantstorenderframe_	= true;
};
