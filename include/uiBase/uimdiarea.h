#ifndef uimdiarea_h
#define uimdiarea_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2008
 RCS:           $Id: uimdiarea.h,v 1.3 2009-07-22 16:01:21 cvsbert Exp $
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
