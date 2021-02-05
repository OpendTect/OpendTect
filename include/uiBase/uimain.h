#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          03/12/1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigeom.h"
#include "color.h"
#include "notify.h"

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
    Color		windowColor() const;
    static void		setIcon(const char* icid);

    int			nrScreens() const;
    uiSize		getScreenSize(int screennr,bool availablesz) const;
    uiSize		desktopSize() const;
			//!<\returns mUdf(int) if unknown

    static uiMain&	theMain();
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
    static IdxPair	getDPI();
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

			//! necessary for uiMain coin initialisation
    virtual void	init( mQtclass(QWidget*) mainwidget )	{}
};


mGlobal(uiBase) bool isMainThread(Threads::ThreadID);
mGlobal(uiBase) bool isMainThreadCurrent();
