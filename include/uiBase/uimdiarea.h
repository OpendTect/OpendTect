#ifndef uimdiarea_h
#define uimdiarea_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2008
 RCS:           $Id: uimdiarea.h,v 1.2 2009-01-09 04:26:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uiobj.h"

class BufferStringSet;
class IOPar;
class uiMdiAreaBody;

class QMdiArea;
class QMdiSubWindow;

class uiMdiAreaGroup : public uiGroup
{
public:
    			uiMdiAreaGroup(const char* nm=0);
			~uiMdiAreaGroup()	{}

    void		setTitle(const char*);
    const char*		getTitle() const;

    void		setIcon(const char* img[]);

    NotifierAccess&	closed();
    QMdiSubWindow*	qWidget();

    Notifier<uiMdiAreaGroup> changed;

protected:
    QMdiSubWindow*	qmdisubwindow_;

};


mClass uiMdiArea : public uiObject
{ 	
friend class		uiMdiAreaBody;
friend class		i_MdiAreaMessenger;
public:
			uiMdiArea(uiParent*,const char* nm="uiMdiArea");

    void		tileHorizontal();
    void		tileVertical();
    void		tile();
    void		cascade();
    void		addGroup(uiMdiAreaGroup*);
    void		closeAll();

    void		setActiveWin(const char*);
    void		setActiveWin(uiMdiAreaGroup*);
    const char*		getActiveWin() const;
    void		getWindowNames(BufferStringSet&);

    Notifier<uiMdiArea> windowActivated;

protected:

    uiMdiAreaBody&	mkbody(uiParent*,const char*);
    uiMdiAreaBody*	body_;

    ObjectSet<uiMdiAreaGroup> grps_;

    void		grpClosed(CallBacker*);
    void		grpChanged(CallBacker*);
};

#endif
