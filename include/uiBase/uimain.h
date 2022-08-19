#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigeom.h"
#include "color.h"
#include "notify.h"
#include "odpair.h"

mFDQtclass(QApplication)
mFDQtclass(QWidget)
mFDQtclass(QtTabletEventFilter)

class uiMainWin;
class uiFont;
class BufferStringSet;
class KeyboardEventHandler;
class KeyboardEventFilter;


mExpClass(uiBase) uiMain : public CallBacker
{
public:

    static void		preInitForOpenGL();
			//!< call before the uiMain object is constructed
    static bool		reqOpenGL();

			uiMain(int& argc,char** argv);
private:
			uiMain(mQtclass(QApplication*));
    void		init(mQtclass(QApplication*),int& argc,char **argv);
    void		preInit();

public:

    virtual		~uiMain();

    virtual int		exec();
    void		restart();
    void		exit(int retcode=0);
    void*		thread();

    void		getCmdLineArgs(BufferStringSet&) const;
    void		setTopLevel(uiMainWin*);
    uiMainWin*		topLevel()			{ return mainobj_; }
    void		setFont(const uiFont&,bool passtochildren);
    const uiFont*	font();
    OD::Color		windowColor() const;
    static void		setIcon(const char* icid);

    int			nrScreens() const;
    uiSize		getScreenSize(int screennr,bool availablesz) const;
    uiSize		desktopSize() const;
			//!<\returns mUdf(int) if unknown
    double		getDevicePixelRatio(int screennr) const;

    static uiMain&	instance();
    mDeprecated("Use instance()")
    static uiMain&	theMain()		{ return instance(); }
    static void		setXpmIconData( const char** xpmdata );
    static const char**	XpmIconData;
    static void		cleanQtOSEnv();

    static void		repaint();

    /*!  Processes pending events for maxtime milliseconds
     *   or until there are no more events to process, whichever is shorter.
     *   Only works after themain has been constructed.
     */
    static void		processEvents(int msec=3000);

    static KeyboardEventHandler& keyboardEventHandler();
    static OD::Pair<int,int> getDPI(int screennr=-1);
    static int		getMinDPI();
    static double	getDefZoomLevel();

    static void		useNameToolTip(bool);
    static bool		isNameToolTipUsed();
    static void		formatNameToolTipString(BufferString&);
    static bool		directRendering();

protected:

    static uiMain*	themain_;
    uiMainWin*		mainobj_;

    static mQtclass(QApplication*)  app_;
    static const uiFont*  font_;

    static mQtclass(QtTabletEventFilter*)  tabletfilter_;
    static KeyboardEventHandler*	keyhandler_;
    static KeyboardEventFilter*		keyfilter_;

			//! necessary for uiMain coin initialization
    virtual void	init( mQtclass(QWidget*) mainwidget )	{}
};


mGlobal(uiBase) bool isMainThread(Threads::ThreadID);
mGlobal(uiBase) bool isMainThreadCurrent();
