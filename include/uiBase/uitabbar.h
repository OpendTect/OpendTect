#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

class uiTabBarBody;
class uiGroup;
class uiTabBody;
class uiTab;


//! TabBar widget only. Normally you'd want to use the uiTabStack class.
mExpClass(uiBase) uiTabBar : public uiObject
{
friend class		i_tabbarMessenger;
friend class		uiTabStack;
public:
			uiTabBar(uiParent*,const char* nm,
				 const CallBack* cb=0);

    int			addTab(uiTab*);
    void		removeTab(uiTab*);
    void		removeTab(uiGroup*);

    void		setTabEnabled(int idx,bool yn);
    bool		isTabEnabled(int idx) const;

    void		setTabVisible(int idx,bool yn);
    bool		isTabVisible(int idx) const;

    void		setCurrentTab(int idx);
    int			currentTabId() const;
    uiString		textOfTab(int idx) const;
    void		setTabIcon(int idx,const char*);
    void		setTabsClosable(bool closable);
    void		showCloseButton(int idx,bool yn,bool shrink=false);
    int			insertTab(uiTab*,int index);
    void		setTabText(int idx,const QString&);

    int			size() const;

    Notifier<uiTabBar>  selected;
    CNotifier<uiTabBar,int>  tabToBeClosed;

    int			indexOf(const uiGroup*) const;
    int			indexOf(const uiTab*) const;
    uiGroup*		page(int idx) const;

protected:
			~uiTabBar();

    uiTabBarBody*	body_;
    uiTabBarBody&	mkbody(uiParent*,const char*);

    ObjectSet<uiTab>	tabs_;
};


mExpClass(uiBase) uiTab
{
friend class		uiTabBar;
public:
			uiTab(uiGroup&,const uiString& caption);
    virtual		~uiTab();

    void		setCaption(const uiString&);
    const uiString&	getCaption() const		{ return caption_; }


    uiGroup&		group() 			{ return grp_; }
    const uiGroup&	group() const			{ return grp_; }

protected:

    uiGroup&		grp_;
    uiString		caption_;
};
