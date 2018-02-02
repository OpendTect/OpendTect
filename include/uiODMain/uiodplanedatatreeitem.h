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
#include "uiodattribtreeitem.h"
#include "uiodprobeparenttreeitem.h"
#include "oduicommon.h"
#include "datapack.h"
#include "flatview.h"
#include "probe.h"

class uiSliceSelDlg;
class TrcKeyZSampling;
namespace Attrib { class DescID; }
namespace Well { class Data; }


mExpClass(uiODMain) uiODPlaneDataTreeItem : public uiODSceneProbeTreeItem
{ mODTextTranslationClass(uiODPlaneDataTreeItem)
public:

			~uiODPlaneDataTreeItem();

    bool		init();

protected:
			uiODPlaneDataTreeItem(int displayid,OD::SliceType,
					      Probe&);

    virtual void	updateDisplay();
    void		handleObjChanged(const ChangeData&);

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    void		updatePlanePos(CallBacker*);
    void		updatePositionDlg(CallBacker*);
    void		posDlgClosed(CallBacker*);

    void		selChg(CallBacker*);
    void		posChange(CallBacker*);
    void		keyUnReDoPressedCB(CallBacker*);
    void		selectRGBA();

    const OD::SliceType	orient_;
    MenuItem		positionmnuitem_;
    MenuItem		gridlinesmnuitem_;
    MenuItem		addinlitem_;
    MenuItem		addcrlitem_;
    MenuItem		addzitem_;

    uiSliceSelDlg*	positiondlg_;
};



mExpClass(uiODMain) uiODPlaneDataParentTreeItem
			: public uiODSceneProbeParentTreeItem
{ mODTextTranslationClass(uiODPlaneDataParentTreeItem);
public:

			uiODPlaneDataParentTreeItem(const uiString&);

    virtual void	addMenuItems();
    virtual bool	handleSubMenu(int mnuid);
    virtual bool	setPosToBeAddedFromWell(const Well::Data&)
							{ return false;}
    static uiString	sAddAtWellLocation();
    static int		cAddAtWellLocationMenuID()	{ return 55; }

protected:

    TrcKeyZSampling	probetobeaddedpos_;

    virtual bool	canAddFromWell() const	{ return false; }
    virtual bool	setProbeToBeAddedParams(int mnuid);
    virtual void	setDefaultPosToBeAdded()	= 0;

};


mExpClass(uiODMain) uiODInlineParentTreeItem
			: public uiODPlaneDataParentTreeItem
{   mODTextTranslationClass(uiODInlineParentTreeItem);
    mDefineItemMembers( InlineParent, PlaneDataParentTreeItem, SceneTreeTop );
    mMenuOnAnyButton;

    bool		canShowSubMenu() const;
    bool		canAddFromWell() const	{ return true; }
    Probe*		createNewProbe() const;
    virtual bool	setPosToBeAddedFromWell(const Well::Data&);
    const char*		childObjTypeKey() const;
    uiPresManagedTreeItem* addChildItem(const Presentation::ObjInfo&);

protected:

    virtual void		setDefaultPosToBeAdded();

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

			uiODInlineTreeItem(Probe&,int displayid=-1);

protected:

    const char*		parentType() const
			{ return typeid(uiODInlineParentTreeItem).name(); }

};


mExpClass(uiODMain) uiODInlineAttribTreeItem : public uiODAttribTreeItem
{
public:
				uiODInlineAttribTreeItem(const char*);
    static void			initClass();
    static uiODDataTreeItem*	create(ProbeLayer&);
};


mExpClass(uiODMain) uiODCrosslineParentTreeItem
			: public uiODPlaneDataParentTreeItem
{   mODTextTranslationClass(uiODCrossineParentTreeItem);
    mDefineItemMembers( CrosslineParent, SceneProbeParentTreeItem,SceneTreeTop);
    mMenuOnAnyButton;

    bool		canShowSubMenu() const;
    bool		canAddFromWell() const	{ return true; }
    const char*		childObjTypeKey() const;
    uiPresManagedTreeItem* addChildItem(const Presentation::ObjInfo&);
    Probe*		createNewProbe() const;
    virtual bool	setPosToBeAddedFromWell(const Well::Data&);

protected:

    virtual void	setDefaultPosToBeAdded();

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
			uiODCrosslineTreeItem(Probe&,int diplayid=-1);

protected:

    const char*		parentType() const
			{ return typeid(uiODCrosslineParentTreeItem).name(); }

};


mExpClass(uiODMain) uiODCrosslineAttribTreeItem : public uiODAttribTreeItem
{
public:

			uiODCrosslineAttribTreeItem(const char*);
    static void		initClass();
    static uiODDataTreeItem* create(ProbeLayer&);

};


mExpClass(uiODMain) uiODZsliceParentTreeItem
			: public uiODPlaneDataParentTreeItem
{   mODTextTranslationClass(uiODZsliceParentTreeItem);
    mDefineItemMembers( ZsliceParent, SceneProbeParentTreeItem, SceneTreeTop );
    mMenuOnAnyButton;

    bool		canShowSubMenu() const;
    const char*		childObjTypeKey() const;
    uiPresManagedTreeItem* addChildItem(const Presentation::ObjInfo&);
    Probe*		createNewProbe() const;

protected:

    virtual void	setDefaultPosToBeAdded();

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
{ mODTextTranslationClass(uiODZsliceTreeItem);
public:
			uiODZsliceTreeItem(Probe&,int displayid=-1);

protected:
    const char*		parentType() const
			{ return typeid(uiODZsliceParentTreeItem).name(); }
    uiString		createDisplayName() const;
};


mExpClass(uiODMain) uiODZsliceAttribTreeItem : public uiODAttribTreeItem
{
public:

			uiODZsliceAttribTreeItem(const char*);
    static void		initClass();
    static uiODDataTreeItem* create(ProbeLayer&);

};
