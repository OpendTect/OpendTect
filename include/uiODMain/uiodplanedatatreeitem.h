#ifndef uiodplanedatatreeitem_h
#define uiodplanedatatreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodplanedatatreeitem.h,v 1.19 2012-09-13 18:42:24 cvsnanne Exp $
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddisplaytreeitem.h"

class uiSliceSelDlg;
class CubeSampling;
namespace Attrib { class DescID; }
namespace Well { class Data; }


mClass(uiODMain) uiODPlaneDataTreeItem : public uiODDisplayTreeItem
{
public:
    enum Type		{ Default, Empty, RGBA };
    enum Orientation	{ Inline, Crossline, ZSlice };

    			uiODPlaneDataTreeItem(int displayid,Orientation,Type);
			~uiODPlaneDataTreeItem();
    bool		init();
    void		setAtWellLocation(const Well::Data&);
    bool		displayDefaultData();

protected:
    BufferString	createDisplayName() const;
    bool		getDefaultDescID(Attrib::DescID&);

    void		addToToolBarCB(CallBacker*);
    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    void		updatePlanePos(CallBacker*);
    void		updatePositionDlg(CallBacker*);
    void		posDlgClosed(CallBacker*);
    void		keyPressCB(CallBacker*);
    void		movePlane(bool forward,int step=1);

    void		selChg(CallBacker*);
    void		posChange(CallBacker*);
    void		movePlaneAndCalcAttribs(const CubeSampling&);

    const Orientation	orient_;
    const Type		type_;
    MenuItem		positionmnuitem_;
    MenuItem		gridlinesmnuitem_;
    uiSliceSelDlg*	positiondlg_;
};



mDefineItem( InlineParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass(uiODMain) uiODInlineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODInlineParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mClass(uiODMain) uiODInlineTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODInlineTreeItem(int displayid,Type);

protected:
    const char*		parentType() const
			{ return typeid(uiODInlineParentTreeItem).name(); }
};


mDefineItem( CrosslineParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass(uiODMain) uiODCrosslineTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODCrosslineParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mClass(uiODMain) uiODCrosslineTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODCrosslineTreeItem(int displayid,Type);

protected:
    const char*		parentType() const
			{ return typeid(uiODCrosslineParentTreeItem).name(); }
};


mDefineItem( ZsliceParent, TreeItem, TreeTop, mShowMenu mMenuOnAnyButton );


mClass(uiODMain) uiODZsliceTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODZsliceParentTreeItem; }
    uiTreeItem*		createForVis(int visid,uiTreeItem*) const;
};


mClass(uiODMain) uiODZsliceTreeItem : public uiODPlaneDataTreeItem
{
public:
    			uiODZsliceTreeItem(int displayid,Type);

protected:
    const char*		parentType() const
			{ return typeid(uiODZsliceParentTreeItem).name(); }
};


#endif

