#ifndef uiodparenttreeitem_h
#define uiodparenttreeitem_h

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
    bool		init();

protected:
    uiODApplMgr*	applMgr();
    int			sceneID() const;

    virtual void	addChildItem(const DBKey&)			=0;
};

#endif
