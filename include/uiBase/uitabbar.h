#ifndef uitabbar_h
#define uitabbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          14/02/2003
 RCS:           $Id: uitabbar.h,v 1.14 2008-01-03 12:16:03 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

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
    void		removeTab(uiTab*);
    void		removeTab(uiGroup*);

    void		setTabEnabled(int id,bool);
    bool		isTabEnabled(int id) const;

    void		setCurrentTab(int id);
    int			currentTabId() const;
    
    int			size() const;

    Notifier<uiTabBar>  selected;

    int			idOf(const uiGroup*) const;
    int			idOf(const uiTab*) const;
    uiGroup*		page(int id) const;

protected:

    uiTabBarBody*	body_;
    uiTabBarBody&	mkbody(uiParent*,const char*);

    ObjectSet<uiTab>	tabs_;
};


class uiTab : public NamedObject
{
friend class		uiTabBar;
public:
			uiTab(uiGroup&);

    uiGroup&		group()		{ return grp_; }
    const uiGroup&	group() const	{ return grp_; }

protected:

    uiGroup&		grp_;
};

#endif
