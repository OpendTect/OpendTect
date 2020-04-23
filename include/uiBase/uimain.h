#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          03/12/1999
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigeom.h"
#include "color.h"

mFDQtclass(QApplication)
mFDQtclass(QDesktopWidget)
mFDQtclass(QWidget)
mFDQtclass(QtTabletEventFilter)

class BufferStringSet;
class CommandLineParser;
class KeyboardEventHandler;
class KeyboardEventFilter;
class uiMainWin;
class uiFont;


mExpClass(uiBase) uiMain : public CallBacker
{
public:

    static void		preInitForOpenGL();
			//!< call before the uiMain object is constructed

			uiMain(const uiString& appname=uiString::empty(),
				const uiString& orgnm=uiString::empty());
			    //!< defaults are uiStrings sOpendTect(), sdGB()

    CommandLineParser&	commandLineParser()	{ return *clp_; }
    static CommandLineParser& CLP()		{ return *theMain().clp_; }

public:

    virtual		~uiMain();

    virtual int		exec();
    void		exit(int retcode=0);
    void		restart(); // if it returns, it failed.
    void*		thread();

    void		getCmdLineArgs(BufferStringSet&) const;
    void		setTopLevel(uiMainWin*);
    uiMainWin*		topLevel()			{ return mainobj_; }
    void		setFont(const uiFont&,bool passtochildren);
    const uiFont*	font();
    Color		windowColor() const;
    static void		setIconFileName(const char* full_path); // before start
    static void		setIcon(const char* icid); // when running
    static const char*	iconFileName();
    bool		setStyleSheet(const char*);

    int			nrScreens() const;
    const char*		getScreenName(int screennr) const;
    uiSize		getScreenSize(int screennr,bool availablesz) const;
    uiSize		desktopSize() const;
			//!<\returns mUdf(int) if unknown
    double		getDevicePixelRatio(int screennr) const;

    static uiMain&	theMain();
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
    static void		setXpmIconData(const char**);
    static bool		directRendering();

protected:

    static uiMain*	themain_;
    uiMainWin*		mainobj_		= 0;
    CommandLineParser*	clp_;

    static mQtclass(QApplication*)		app_;
    static const uiFont*			font_;

    static mQtclass(QtTabletEventFilter*) tabletfilter_;
    static KeyboardEventHandler*	keyhandler_;
    static KeyboardEventFilter*		keyfilter_;
    mQtclass(QDesktopWidget*)		qdesktop_		= 0;

    void		languageChangeCB(CallBacker*);
    static void		updateAllToolTips();

private:

			uiMain(mQtclass(QApplication*));
    void		preInit();
    void		init(mQtclass(QApplication*));

};


mGlobal(uiBase) bool isMainThread(Threads::ThreadID);
mGlobal(uiBase) bool isMainThreadCurrent();
