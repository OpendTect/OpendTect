#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigroup.h"

class uiTabBar;


/* \brief Provides a way to create tabs with associated uiGroups.

    uiTabBar provides only the tabs. uiTabStack forms a system that
    combines such a tab-bar with uiGroups by which selection of a tab
    automatically makes the associated uiGroup visible.

    uiGroups that are to be added to the stack, *must* use the
    tabGroup() as a parent. Otherwise, you will get a programmer error
    message complaining that the added group "Must be child or
    child-of-child."

*/
mExpClass(uiBase) uiTabStack : public uiGroup
{
public:
			uiTabStack(uiParent*,const char* nm,
				   bool manage=true);
			~uiTabStack();

			// use this as parent for groups to add
    uiGroup*		tabGroup()			{ return tabgrp_; }

    void		addTab(uiGroup*,
			       const uiString& =uiString::emptyString());
    void		removeTab(uiGroup*);

    void		setTabEnabled(int idx,bool yn);
    bool		isTabEnabled(int idx) const;
    void		setTabEnabled(uiGroup*,bool yn);
    bool		isTabEnabled(uiGroup*) const;

    void		setTabVisible(uiGroup*,bool yn);
    bool		isTabVisible(uiGroup*) const;

    void		setCurrentPage(int idx);
    void		setCurrentPage(uiGroup*);
    void		setCurrentPage(const char* grpnm);
    void		setTabIcon(int idx,const char* icnnm);
    void		setTabIcon(uiGroup*,const char* icnnm);
    void		setTabsClosable(bool closable);
    void		showCloseButton(uiGroup*,bool yn,bool shrink=false);
    int			insertTab(uiGroup*, int index,
				  const uiString& =uiString::emptyString());
    void		setTabText(int idx, const char* nm);

    uiGroup*		currentPage() const;
    uiGroup* 		page(int idx) const;
    int			indexOf(uiGroup*) const;
    int			currentPageId() const;

    int			size() const;

    NotifierAccess&	selChange();
    CNotifier<uiTabStack,int>  tabToBeClosed;
    Notifier<uiTabStack>	tabClosed;

protected:

    uiTabBar*		tabbar_;
    uiGroup*		tabgrp_;

    void		tabSel(CallBacker* cb);
    void		tabCloseCB(CallBacker* cb);
};
