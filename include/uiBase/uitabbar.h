#ifndef uitabbar_h
#define uitabbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          14/02/2003
 RCS:           $Id: uitabbar.h,v 1.13 2007-02-14 12:38:01 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class ioPixmap;
class uiTabBarBody;
class uiGroup;
class uiTabBody;
class uiTab;

#ifdef USEQT3
# define mRemoveTabArg int index
#else
# define mRemoveTabArg uiTab* tab
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
#ifdef USEQT3
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

    int			idOf(const uiGroup*) const;
#ifndef USEQT3
    int			idOf(const uiTab*) const;
#endif
    uiGroup*		page(int id) const;

protected:

    uiTabBarBody*	body_;
    uiTabBarBody&	mkbody(uiParent*, const char*);

    ObjectSet<uiTab>	tabs_;
};


class uiTab : public NamedObject
{
friend class		uiTabBar;
public:
			uiTab(uiGroup&);
#ifdef USEQT3
    int			id() const;
    void		setName(const char*);
#endif

    uiGroup&		group()		{ return grp_; }
    const uiGroup&	group() const	{ return grp_; }

protected:

#ifdef USEQT3
    uiTabBody&		body_;
#endif
    uiGroup&		grp_;
};

#endif
