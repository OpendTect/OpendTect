#ifndef uiodplanedatatreeitem_h
#define uiodplanedatatreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"
#include "oduicommon.h"

class uiSliceSelDlg;
class TrcKeyZSampling;
namespace Attrib { class DescID; }
namespace Well { class Data; }


mExpClass(uiODMain) uiODPlaneDataTreeItem : public uiODDisplayTreeItem
{ mODTextTranslationClass(uiODPlaneDataTreeItem)
public:

    enum Type		{ Empty, Select, Default, RGBA };

			~uiODPlaneDataTreeItem();

    bool		init();
    void		setAtWellLocation(const Well::Data&);
    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    bool		displayDefaultData();
    bool		displayGuidance();
    bool		displayDataFromDesc(const Attrib::DescID&,bool stored);
    bool		displayDataFromOther(int visid);

    static uiString	sAddEmptyPlane();
    static uiString	sAddAndSelectData();
    static uiString	sAddDefaultData();
    static uiString	sAddColorBlended();
    static uiString	sAddAtWellLocation();

protected:
			uiODPlaneDataTreeItem(int displayid,OD::SliceType,Type);

    uiString		createDisplayName() const;

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    void		updatePlanePos(CallBacker*);
    void		updatePositionDlg(CallBacker*);
    void		posDlgClosed(CallBacker*);
    void		keyPressCB(CallBacker*);
    void		movePlane(bool forward,int step=1);

    void		selChg(CallBacker*);
    void		posChange(CallBacker*);
    void		movePlaneAndCalcAttribs(const TrcKeyZSampling&);
    void		keyUnReDoPressedCB(CallBacker*);

    const OD::SliceType	orient_;
    const Type		type_;
    MenuItem		positionmnuitem_;
    MenuItem		gridlinesmnuitem_;
    MenuItem		addinlitem_;
    MenuItem		addcrlitem_;
    MenuItem		addzitem_;

    uiSliceSelDlg*	positiondlg_;
};



mExpClass(uiODMain) uiODInlineParentTreeItem : public uiODTreeItem
{   mODTextTranslationClass(uiODInlineParentTreeItem);
    mDefineItemMembers( InlineParent, TreeItem, TreeTop );
    mShowMenu;
    mMenuOnAnyButton;
};


mExpClass(uiODMain) uiODInlineTreeItemFactory : public uiODTreeItemFactory
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

protected:
    const char*		parentType() const
			{ return typeid(uiODInlineParentTreeItem).name(); }
};


mExpClass(uiODMain) uiODCrosslineParentTreeItem : public uiODTreeItem
{   mODTextTranslationClass(uiODCrossineParentTreeItem);
    mDefineItemMembers( CrosslineParent, TreeItem, TreeTop );
    mShowMenu;
    mMenuOnAnyButton;
};


mExpClass(uiODMain) uiODCrosslineTreeItemFactory : public uiODTreeItemFactory
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

protected:
    const char*		parentType() const
			{ return typeid(uiODCrosslineParentTreeItem).name(); }
};



mExpClass(uiODMain) uiODZsliceParentTreeItem : public uiODTreeItem
{   mODTextTranslationClass(uiODZsliceParentTreeItem);
    mDefineItemMembers( ZsliceParent, TreeItem, TreeTop );
    mShowMenu;
    mMenuOnAnyButton;
};


mExpClass(uiODMain) uiODZsliceTreeItemFactory : public uiODTreeItemFactory
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

protected:
    const char*		parentType() const
			{ return typeid(uiODZsliceParentTreeItem).name(); }
};


#endif
