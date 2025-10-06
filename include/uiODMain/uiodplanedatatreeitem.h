#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"

#include "datapack.h"
#include "flatview.h"
#include "odcommonenums.h"

class uiSliceSelDlg;
class RegularSeisDataPack;
class TrcKeyZSampling;
namespace Attrib { class DescID; }
namespace visSurvey { class PlaneDataDisplay; }
namespace Well { class Data; }


mExpClass(uiODMain) uiODPlaneDataTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPlaneDataTreeItem)
public:

    enum Type		{ Empty, Select, Default, RGBA };

    bool		init() override;
    void		setAtWellLocation(const Well::Data&);
    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    bool		displayDefaultData();
    bool		displayGuidance();
    bool		displayDataFromDesc(const Attrib::DescID&,bool stored);
    bool		displayDataFromDataPack(RegularSeisDataPack&,
					    const Attrib::SelSpec&,
					    const FlatView::DataDispPars::VD&);
    bool		displayDataFromOther(const VisID&);

    static uiString	sAddEmptyPlane();
    static uiString	sAddAndSelectData();
    static uiString	sAddDefaultData();
    static uiString	sAddColorBlended();
    static uiString	sAddAtWellLocation();

protected:
			uiODPlaneDataTreeItem(const VisID&,OD::SliceType,Type);
			~uiODPlaneDataTreeItem();

    uiString		createDisplayName() const override;

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;

    void		updatePlanePos(CallBacker*);
    void		updatePositionDlg(CallBacker*);
    void		posDlgClosed(CallBacker*);
    void		keyPressCB(CallBacker*) override;
    void		movePlane(bool forward,int step=1);

    void		selChg(CallBacker*);
    void		posChange(CallBacker*);
    void		sliderReleasedCB(CallBacker*);
    void		movePlaneAndCalcAttribs(const TrcKeyZSampling&,
						bool dispplane=true);
    void		keyUnReDoPressedCB(CallBacker*);

    const OD::SliceType	orient_;
    const Type		type_;
    MenuItem		positionmnuitem_;
    MenuItem		gridlinesmnuitem_;
    MenuItem		addinlitem_;
    MenuItem		addcrlitem_;
    MenuItem		addzitem_;

    ConstRefMan<visSurvey::PlaneDataDisplay> getDisplay() const;
    RefMan<visSurvey::PlaneDataDisplay> getDisplay();

    WeakPtr<visSurvey::PlaneDataDisplay> pdd_;

    uiSliceSelDlg*	positiondlg_ = nullptr;
};


// In-line items
mExpClass(uiODMain) uiODInlineParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODInlineParentTreeItem);
public:
			uiODInlineParentTreeItem();

    static CNotifier<uiODInlineParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODInlineParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
};


mExpClass(uiODMain) uiODInlineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODInlineParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODInlineTreeItem : public uiODPlaneDataTreeItem
{
public:
			uiODInlineTreeItem(const VisID&,Type);

protected:
			~uiODInlineTreeItem();

    const char*		parentType() const override
			{ return typeid(uiODInlineParentTreeItem).name(); }
};


// Cross-line items
mExpClass(uiODMain) uiODCrosslineParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODCrosslineParentTreeItem);
public:
			uiODCrosslineParentTreeItem();

    static CNotifier<uiODCrosslineParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODCrosslineParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
};


mExpClass(uiODMain) uiODCrosslineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODCrosslineParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODCrosslineTreeItem : public uiODPlaneDataTreeItem
{
public:
			uiODCrosslineTreeItem(const VisID&,Type);

protected:
			~uiODCrosslineTreeItem();

    const char*		parentType() const override
			{ return typeid(uiODCrosslineParentTreeItem).name(); }
};


// Z slice items
mExpClass(uiODMain) uiODZsliceParentTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODZsliceParentTreeItem);
public:
			uiODZsliceParentTreeItem();

    static CNotifier<uiODZsliceParentTreeItem,uiMenu*>& showMenuNotifier();

protected:
			~uiODZsliceParentTreeItem();

    const char*		iconName() const override;
    bool		showSubMenu() override;
};


mExpClass(uiODMain) uiODZsliceTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const override { return typeid(*this).name(); }
    uiTreeItem*		create() const override
			{ return new uiODZsliceParentTreeItem; }
    uiTreeItem*		createForVis(const VisID&,uiTreeItem*) const override;
};


mExpClass(uiODMain) uiODZsliceTreeItem : public uiODPlaneDataTreeItem
{
public:
			uiODZsliceTreeItem(const VisID&,Type);

protected:
			~uiODZsliceTreeItem();

    const char*		parentType() const override
			{ return typeid(uiODZsliceParentTreeItem).name(); }
};
