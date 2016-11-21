#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodprmantreeitem.h"
#include "odpresentationmgr.h"

class uiODApplMgr;

mExpClass(uiODMain) uiODSceneParentTreeItem : public uiODPrManagedParentTreeItem
{ mODTextTranslationClass(uiODSceneParentTreeItem)
public:
			uiODSceneParentTreeItem(const uiString&);
    virtual		~uiODSceneParentTreeItem();
    virtual bool	anyButtonClick(uiTreeViewItem*);
    bool		init();
    OD::ViewerID	getViewerID() const;

protected:
    uiODApplMgr*	applMgr() const;
    int			sceneID() const;
    void		setMoreObjectsToDoHint(bool yn);
};
