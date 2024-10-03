#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "visseis2ddisplay.h"

class uiMenu;
class uiTaskRunner;

mExpClass(uiODMain) uiODLine2DParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODLine2DParentTreeItem)
public:
			uiODLine2DParentTreeItem();

    static const char*	sKeyRightClick();
    static const char*	sKeyUnselected();

    static CNotifier<uiODLine2DParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODLine2DParentTreeItem();

    bool		loadDefaultData();
    bool		selectLoadAttribute(const TypeSet<Pos::GeomID>&,
			    const char* attrnm=sKeyRightClick(),int attridx=-1);
    void		setTopAttribName(const char*);

    const char*		iconName() const override;
    bool		showSubMenu() override;
    bool		handleSubMenu(int);

    uiMenu*		replaceattritm_ = nullptr;
    uiMenu*		removeattritm_	= nullptr;
    uiMenu*		dispattritm_	= nullptr;
    uiMenu*		hideattritm_	= nullptr;
    uiMenu*		editcoltabitm_	= nullptr;
};



mExpClass(uiODMain) Line2DTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(Line2DTreeItemFactory)
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODLine2DParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiOD2DLineTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiOD2DLineTreeItem)
public:
			uiOD2DLineTreeItem(const Pos::GeomID&,
					   const VisID& =VisID::udf(),
					   bool rgba=false);

    bool		displayDefaultData();
    bool		addStoredData(const char*,int component,uiTaskRunner&);
    void		addAttrib(const Attrib::SelSpec&,uiTaskRunner&);
    void		showLineName(bool);
    void		setZRange(const Interval<float>);
    void		removeAttrib(const char*);

    Pos::GeomID		getGeomID() const { return geomid_; }

protected:
			~uiOD2DLineTreeItem();

    bool		init() override;
    const char*		parentType() const override;
    uiString		createDisplayName() const override;

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const override;

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		getNewData(CallBacker*);

private:

    Pos::GeomID		geomid_;
    MenuItem		linenmitm_;
    MenuItem		panelitm_;
    MenuItem		polylineitm_;
    MenuItem		positionitm_;
    bool		rgba_;

    ConstRefMan<visSurvey::Seis2DDisplay> getDisplay() const;
    RefMan<visSurvey::Seis2DDisplay> getDisplay();

    WeakPtr<visSurvey::Seis2DDisplay> seis2ddisplay_;
};


mExpClass(uiODMain) uiOD2DLineSetAttribItem : public uiODAttribTreeItem
{ mODTextTranslationClass(uiOD2DLineSetAttribItem)
public:
			uiOD2DLineSetAttribItem(const char* parenttype);

    bool		displayStoredData(const char*,int component,
					  uiTaskRunner&);
    void		setAttrib(const Attrib::SelSpec&,
				  uiTaskRunner&);
    void		clearAttrib();

protected:
			~uiOD2DLineSetAttribItem();

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;

    MenuItem		storeditm_;
    MenuItem		steeringitm_;
    MenuItem		zattritm_;
    MenuItem		attrnoneitm_;
};
