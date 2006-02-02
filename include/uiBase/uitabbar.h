#ifndef uitabbar_h
#define uitabbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          14/02/2003
 RCS:           $Id: uitabbar.h,v 1.10 2006-02-02 09:52:59 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class ioPixmap;
class uiTabBarBody;
class uiGroup;
class uiTabBody;
class uiTab;

#ifdef USEQT4
# define mRemoveTabArg uiTab* tab
#else
# define mRemoveTabArg int index
#endif


//! TabBar widget only. Normally you'd want to use the uiTabStack class.
class uiTabBar : public uiObject
{
friend class		i_tabbarMessenger;
friend class		uiTabStack;
public:
			uiTabBar(uiParent*,const char* nm,
				 const CallBack* cb=0);

    int			addTab(uiTab*);
#ifndef USEQT4
    int			insertTab(uiTab*,int index=-1);
#endif
    void		removeTab(mRemoveTabArg);
    void		removeTab(uiGroup*);

    void		setTabEnabled(int id,bool);
    bool		isTabEnabled(int id) const;

    void		setCurrentTab(int id);
    int			currentTabId() const;
    
    int			size() const;

    Notifier<uiTabBar>  selected;

    int			idOf(uiGroup* grp) const;
#ifdef USEQT4
    int			idOf(uiTab* tab) const;
#endif
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
#ifndef USEQT4
    int			id();
    int			setName(const char*);
#endif

    uiGroup&		group()		{ return grp_; }

protected:

#ifndef USEQT4
    uiTabBody&		body_;
#endif
    uiGroup&		grp_;
};

#endif
