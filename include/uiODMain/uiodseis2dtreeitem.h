#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"

class uiTaskRunner;


mExpClass(uiODMain) uiODLine2DParentTreeItem : public uiODSceneTreeItem
{   mODTextTranslationClass(uiODLine2DParentTreeItem)
    mDefineItemMembers( Line2DParent, SceneTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;
			~uiODLine2DParentTreeItem();

    bool		init();
    int			selectionKey() const;
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    void		createMenu(MenuHandler*,bool istb);
    bool                loadDefaultData();
    bool                selectLoadAttribute(const TypeSet<Pos::GeomID>&,
                             const char* attrnm=sKeyRightClick());
    void		setTopAttribName(const char*);
    static const char*  sKeyRightClick();
    static const char*  sKeyUnselected();

    uiVisPartServer*	visserv_;
    MenuItem		additm_;
    MenuItem		create2dgridfrom3ditm_;
    MenuItem		extractfrom3ditm_;
    MenuItem		generate3dcubeitm_;
    MenuItem		addattritm_;
    MenuItem		replaceattritm_;
    MenuItem		removeattritm_;
    MenuItem		dispattritm_;
    MenuItem		hideattritm_;
    MenuItem		editcoltabitm_;
    MenuItem		displayallitm_;
    MenuItem		hideallitm_;
};



mExpClass(uiODMain) Line2DTreeItemFactory : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(Line2DTreeItemFactory);
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODLine2DParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiOD2DLineTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiOD2DLineTreeItem);
public:
			uiOD2DLineTreeItem(Pos::GeomID,int displayid=-1);

    bool		addStoredData(const char*,int component,uiTaskRunner&);
    void		addAttrib(const Attrib::SelSpec&,uiTaskRunner&);
    void		showLineName(bool);
    void		setZRange(const Interval<float>);
    void		removeAttrib(const char*);

    Pos::GeomID		getGeomID() const { return geomid_; }

protected:
			~uiOD2DLineTreeItem();
    bool		init();
    const char*		parentType() const;
    uiString    	createDisplayName() const;

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		getNewData(CallBacker*);

private:

    Pos::GeomID		geomid_;
    MenuItem		linenmitm_;
    MenuItem		panelitm_;
    MenuItem		polylineitm_;
    MenuItem		positionitm_;
};


mExpClass(uiODMain) uiOD2DLineSetAttribItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiOD2DLineSetAttribItem);
public:
			uiOD2DLineSetAttribItem(const char* parenttype);
    bool		displayStoredData(const char*,int component,
					  uiTaskRunner&);
    void		setAttrib(const Attrib::SelSpec&,
				  uiTaskRunner&);
    void		clearAttrib();

protected:
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    MenuItem		storeditm_;
    MenuItem		steeringitm_;
    MenuItem		zattritm_;
    MenuItem		attrnoneitm_;
};

