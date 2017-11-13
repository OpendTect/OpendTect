#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodscenetreeitem.h"
#include "odpresentationmgr.h"

class uiODApplMgr;

mExpClass(uiODMain) uiODSceneParentTreeItem
			    : public uiPresManagedParentTreeItem
{ mODTextTranslationClass(uiODSceneParentTreeItem)
public:

			uiODSceneParentTreeItem(const uiString&);
			~uiODSceneParentTreeItem();

    bool		init();
    int			sceneID() const;

protected:

    uiODApplMgr*	applMgr() const;
    void		setMoreObjectsToDoHint(bool yn);

    virtual ViewerID	viewerID() const;
    virtual bool	anyButtonClick(uiTreeViewItem*);

};
