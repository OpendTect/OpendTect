#ifndef uitabbar_h
#define uitabbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          14/02/2003
 RCS:           $Id: uitabbar.h,v 1.7 2005-01-13 13:42:35 arend Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class ioPixmap;
class uiTabBarBody;
class uiGroup;
class uiTabBody;
class uiTab;

//! TabBar widget only. Normally you'd want to use the uiTabStack class.
class uiTabBar : public uiObject
{
friend class		i_tabbarMessenger;
friend class		uiTabStack;
public:
			uiTabBar(uiParent*,const char* nm,
				 const CallBack* cb=0);

    int			addTab(uiTab*);
    int			insertTab(uiTab*,int index=-1);
    void		removeTab(int index);
    void		removeTab(uiGroup*);

    void		setTabEnabled(int id,bool);
    bool		isTabEnabled(int id) const;

    void		setCurrentTab(int id);
    int			currentTabId() const;
    int			keyboardFocusTabId() const;
    
    int			size() const;

    Notifier<uiTabBar>  selected;

    int			idOf(uiGroup* grp) const;
    uiGroup*		page(int id) const;

protected:

    uiTabBarBody*	body_;
    uiTabBarBody&	mkbody(uiParent*, const char*);

    ObjectSet<uiTab>	tabs_;
};


class uiTab : public UserIDObject
{
friend class		uiTabBar;
public:
			uiTab(uiGroup&);

    int 		id();

    const uiTabBody&	body() const	{ return body_; }
    uiGroup&		group()		{ return grp_; }

protected:

    uiTabBody&		body_;
    uiGroup&		grp_;
};

#endif
