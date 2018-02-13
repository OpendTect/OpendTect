#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

class VW2DPickSet;
namespace Pick{ class Set; }



mExpClass(uiODMain) uiODVw2DPickSetParentTreeItem
			: public uiODVw2DParentTreeItem
{ mODTextTranslationClass(uiODVw2DPickSetParentTreeItem);
public:

				uiODVw2DPickSetParentTreeItem();
				~uiODVw2DPickSetParentTreeItem();

    bool			showSubMenu();
    const char*			childObjTypeKey() const;
    uiPresManagedTreeItem*	addChildItem(const Presentation::ObjInfo&);

protected:

    bool			init();
    const char*			iconName() const;
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
};


mExpClass(uiODMain)
uiODVw2DPickSetTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:

    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DPickSetParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,int visid) const;

};


mExpClass(uiODMain) uiODVw2DPickSetTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DPickSetTreeItem)
public:

			uiODVw2DPickSetTreeItem(Pick::Set&);
			~uiODVw2DPickSetTreeItem();

    bool		showSubMenu();
    bool		select();
    const Vw2DDataObject* vw2DObject() const;
    virtual void	doSave();
    virtual void	doSaveAs();

    void		handleItemCheck(bool triggervwreq);
    PresInfo*		getObjPrInfo() const;

protected:

    const char*		parentType() const
			{ return typeid(uiODVw2DPickSetParentTreeItem).name(); }
    bool		init();
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		setChangedCB(CallBacker*);
    void		displayMiniCtab();


    Pick::Set&		pickset_;
    VW2DPickSet*	vw2dpickset_;

};
