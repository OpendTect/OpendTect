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
#include "uiodprobeparenttreeitem.h"
#include "oduicommon.h"
#include "datapack.h"
#include "flatview.h"
#include "probe.h"

class uiSliceSelDlg;
class TrcKeyZSampling;
namespace Attrib { class DescID; }
namespace Well { class Data; }


mExpClass(uiODMain) uiODPlaneDataTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPlaneDataTreeItem)
public:

			~uiODPlaneDataTreeItem();

    bool		init();
    void		setAtWellLocation(const Well::Data&);
    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    OD::ObjPresentationInfo* getObjPRInfo() const;

protected:
			uiODPlaneDataTreeItem(int displayid,OD::SliceType,
					      Probe&);

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

    Probe&		probe_;
    const OD::SliceType	orient_;
    MenuItem		positionmnuitem_;
    MenuItem		gridlinesmnuitem_;
    MenuItem		addinlitem_;
    MenuItem		addcrlitem_;
    MenuItem		addzitem_;

    uiSliceSelDlg*	positiondlg_;
};



mExpClass(uiODMain) uiODInlineParentTreeItem
			: public uiODProbeParentTreeItem
{   mODTextTranslationClass(uiODInlineParentTreeItem);
    mDefineItemMembers( InlineParent, ProbeParentTreeItem, SceneTreeTop );
    mMenuOnAnyButton;

    bool			canShowSubMenu() const;
    bool			canAddFromWell() const	{ return true; }
    Probe*			createNewProbe() const;
    uiODPrManagedTreeItem*	addChildItem(const OD::ObjPresentationInfo&);
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
			uiODInlineTreeItem(int displayid,Probe&);

protected:
    const char*		parentType() const
			{ return typeid(uiODInlineParentTreeItem).name(); }
};


mExpClass(uiODMain) uiODCrosslineParentTreeItem
			: public uiODProbeParentTreeItem
{   mODTextTranslationClass(uiODCrossineParentTreeItem);
    mDefineItemMembers( CrosslineParent, ProbeParentTreeItem, SceneTreeTop );
    mMenuOnAnyButton;
    bool			canShowSubMenu() const;
    bool			canAddFromWell() const	{ return true; }
    uiODPrManagedTreeItem*	addChildItem(const OD::ObjPresentationInfo&);
    Probe*			createNewProbe() const;
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
			uiODCrosslineTreeItem(int displayid,Probe&);

protected:
    const char*		parentType() const
			{ return typeid(uiODCrosslineParentTreeItem).name(); }
};



mExpClass(uiODMain) uiODZsliceParentTreeItem
			: public uiODProbeParentTreeItem
{   mODTextTranslationClass(uiODZsliceParentTreeItem);
    mDefineItemMembers( ZsliceParent, ProbeParentTreeItem, SceneTreeTop );
    mMenuOnAnyButton;
    bool			canShowSubMenu() const;
    bool			canAddFromWell() const	{ return true; }
    uiODPrManagedTreeItem*	addChildItem(const OD::ObjPresentationInfo&);
    Probe*			createNewProbe() const;
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
			uiODZsliceTreeItem(int displayid,Probe&);

protected:
    const char*		parentType() const
			{ return typeid(uiODZsliceParentTreeItem).name(); }
};
