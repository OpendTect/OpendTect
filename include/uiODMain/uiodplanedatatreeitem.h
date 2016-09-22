#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"
#include "uiodsceneparenttreeitem.h"
#include "oduicommon.h"
#include "datapack.h"
#include "flatview.h"
#include "surveysectionprinfo.h"

class uiSliceSelDlg;
class TrcKeyZSampling;
namespace Attrib { class DescID; }
namespace Well { class Data; }


mExpClass(uiODMain) uiODPlaneDataTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPlaneDataTreeItem)
public:

    enum Type		{ Empty, Select, Default, RGBA, Presentation };

			~uiODPlaneDataTreeItem();

    bool		init();
    void		setSurvSectionID( SurveySectionID sid)
			{ surveysectionid_ = sid; }
    SurveySectionID	survSectionID()
			{ return surveysectionid_; }
    void		setAtWellLocation(const Well::Data&);
    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    bool		displayFromPRInfo();
    bool		displayDefaultData();
    bool		displayGuidance();
    bool		displayDataFromDesc(const Attrib::DescID&,bool stored);
    bool		displayDataFromDataPack(DataPack::ID,
	    				     const Attrib::SelSpec&,
					     const FlatView::DataDispPars::VD&);
    bool		displayDataFromOther(int visid);
    OD::ObjPresentationInfo* getObjPRInfo() const;

    static uiString	sAddEmptyPlane();
    static uiString	sAddAndSelectData();
    static uiString	sAddDefaultData();
    static uiString	sAddColorBlended();
    static uiString	sAddAtWellLocation();

protected:
			uiODPlaneDataTreeItem(int displayid,OD::SliceType,Type);
			uiODPlaneDataTreeItem(int displayid,OD::SliceType,
					 const SurveySectionPresentationInfo&);

    uiString		createDisplayName() const;

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    void		updatePlanePos(CallBacker*);
    void		updatePositionDlg(CallBacker*);
    void		posDlgClosed(CallBacker*);

    void		selChg(CallBacker*);
    void		posChange(CallBacker*);
    void		movePlaneAndCalcAttribs(const TrcKeyZSampling&);
    void		keyUnReDoPressedCB(CallBacker*);
    void		selectRGBA();

    SurveySectionPresentationInfo* initprinfo_;
    SurveySectionID	surveysectionid_;
    const OD::SliceType	orient_;
    const Type		type_;
    MenuItem		positionmnuitem_;
    MenuItem		gridlinesmnuitem_;
    MenuItem		addinlitem_;
    MenuItem		addcrlitem_;
    MenuItem		addzitem_;

    uiSliceSelDlg*	positiondlg_;
};



mExpClass(uiODMain) uiODInlineParentTreeItem
			: public uiODSceneParentTreeItem
{   mODTextTranslationClass(uiODInlineParentTreeItem);
    mDefineItemMembers( InlineParent, SceneParentTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;
    const char*		childObjTypeKey() const;
    void		addChildItem(const OD::ObjPresentationInfo&);
};


mExpClass(uiODMain) uiODInlineTreeItemFactory : public uiODSceneTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODInlineParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODInlineTreeItem : public uiODPlaneDataTreeItem
{
public:
			uiODInlineTreeItem(int displayid,Type);
			uiODInlineTreeItem(int displayid,
					const SurveySectionPresentationInfo&);

protected:
    const char*		parentType() const
			{ return typeid(uiODInlineParentTreeItem).name(); }
};


mExpClass(uiODMain) uiODCrosslineParentTreeItem
			: public uiODSceneParentTreeItem
{   mODTextTranslationClass(uiODCrossineParentTreeItem);
    mDefineItemMembers( CrosslineParent, SceneParentTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;
    const char*		childObjTypeKey() const;
    void		addChildItem(const OD::ObjPresentationInfo&);
};


mExpClass(uiODMain) uiODCrosslineTreeItemFactory
	: public uiODSceneTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODCrosslineParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODCrosslineTreeItem : public uiODPlaneDataTreeItem
{
public:
			uiODCrosslineTreeItem(int displayid,Type);
			uiODCrosslineTreeItem(int displayid,
					const SurveySectionPresentationInfo&);

protected:
    const char*		parentType() const
			{ return typeid(uiODCrosslineParentTreeItem).name(); }
};



mExpClass(uiODMain) uiODZsliceParentTreeItem
			: public uiODSceneParentTreeItem
{   mODTextTranslationClass(uiODZsliceParentTreeItem);
    mDefineItemMembers( ZsliceParent, SceneParentTreeItem, SceneTreeTop );
    mShowMenu;
    mMenuOnAnyButton;
    const char*		childObjTypeKey() const;
    void		addChildItem(const OD::ObjPresentationInfo&);
};


mExpClass(uiODMain) uiODZsliceTreeItemFactory : public uiODSceneTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODZsliceParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODZsliceTreeItem : public uiODPlaneDataTreeItem
{
public:
			uiODZsliceTreeItem(int displayid,Type);
			uiODZsliceTreeItem(int displayid,
					const SurveySectionPresentationInfo&);

protected:
    const char*		parentType() const
			{ return typeid(uiODZsliceParentTreeItem).name(); }
};
