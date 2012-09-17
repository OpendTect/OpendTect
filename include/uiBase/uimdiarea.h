#ifndef uimdiarea_h
#define uimdiarea_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2008
 RCS:           $Id: uimdiarea.h,v 1.6 2010/08/27 02:49:32 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uiobj.h"

class BufferStringSet;
class IOPar;
class uiMdiAreaBody;

class QMdiArea;
class QMdiSubWindow;

mClass uiMdiAreaWindow : public uiGroup
{
public:
    			uiMdiAreaWindow(const char* nm=0);
			~uiMdiAreaWindow()	{}

    void		setTitle(const char*);
    const char*		getTitle() const;

    void		setIcon(const char* img[]);

    void		show();
    void		close();
    void		showMinimized();
    void		showMaximized();
    bool		isMinimized() const;
    bool		isMaximized() const;

    NotifierAccess&	closed();
    QMdiSubWindow*	qWidget();

    Notifier<uiMdiAreaWindow> changed;

protected:
    QMdiSubWindow*	qmdisubwindow_;

};


mClass uiMdiArea : public uiObject
{ 	
friend class		uiMdiAreaBody;
friend class		i_MdiAreaMessenger;
public:
			uiMdiArea(uiParent*,const char* nm="uiMdiArea");
			~uiMdiArea();

    void		tileHorizontal();
    void		tileVertical();
    void		tile();
    void		cascade();
    void		closeAll();

    void		addWindow(uiMdiAreaWindow*);
    uiMdiAreaWindow*	getWindow(const char*);
    const uiMdiAreaWindow* getWindow(const char*) const;

    void		setActiveWin(const char*);
    void		setActiveWin(uiMdiAreaWindow*);
    const char*		getActiveWin() const;
    void		getWindowNames(BufferStringSet&) const;

    Notifier<uiMdiArea> windowActivated;

protected:

    uiMdiAreaBody&	mkbody(uiParent*,const char*);
    uiMdiAreaBody*	body_;

    ObjectSet<uiMdiAreaWindow> grps_;

    void		grpClosed(CallBacker*);
    void		grpChanged(CallBacker*);

public:
			// Temporarily prevent some Qt feedback loop to change
			// active window (solves problems when disabling data
			// tree or when Scenes-menu overlaps workspace)
    bool		paralyse(bool yn);
};

#endif
