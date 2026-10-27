#pragma once
/*+
________________________________________________________________________

 Copyright: (C) 1995-2026 dGB Beheer B.V.
 License:   https://dgbes.com/licensing
________________________________________________________________________

 Qt window that owns the Vulkan surface for a VSG viewer. Adapted from
 vsgQt 0.5.0 (https://github.com/vsg-dev/vsgQt)
 -*/

#include "uivsgmod.h"
#include "commondefs.h"

#include "vsgviewer.h"

#ifdef min
# undef min
#endif
#ifdef max
# undef max
#endif

#include <vsg/app/Window.h>

#include <functional>
#include <utility>

#include <QWindow>

class QEvent;
class QExposeEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QResizeEvent;
class QScreen;
class QWheelEvent;


mClass(uiVSG) dGBVSGWindow : public QWindow
{
public:
	    dGBVSGWindow(vsg::ref_ptr<dGBVSGViewer>,
		    vsg::ref_ptr<vsg::WindowTraits>,
		    QScreen* targetscreen=nullptr);
	    ~dGBVSGWindow();

    void	initializeWindow();
    vsg::ref_ptr<vsg::Window> vsgWindow() const { return vsgwindow_; }
    void	setLeaveCallback(const std::function<void()>&);

protected:

    bool	event(QEvent*) override;
    void	exposeEvent(QExposeEvent*) override;
    void	hideEvent(QHideEvent*) override;
    void	keyPressEvent(QKeyEvent*) override;
    void	keyReleaseEvent(QKeyEvent*) override;
    void	mouseMoveEvent(QMouseEvent*) override;
    void	mousePressEvent(QMouseEvent*) override;
    void	mouseReleaseEvent(QMouseEvent*) override;
    void	resizeEvent(QResizeEvent*) override;
    void	wheelEvent(QWheelEvent*) override;

private:

    void	cleanup();
    int		convertCoord(double) const;
    std::pair<int,int>	mousePosition(const QMouseEvent&) const;
    void	postKey(const QKeyEvent&,bool press);

    vsg::ref_ptr<vsg::WindowTraits> traits_;
    vsg::ref_ptr<dGBVSGViewer>	    viewer_;
    vsg::ref_ptr<vsg::Window>	    vsgwindow_;
    std::function<void()>	    leavecb_;
    bool		initialized_	= false;

};
