#ifndef uibasemapzsliceitem_h
#define uibasemapzsliceitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		March 2015
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"
#include "uibasemap.h"
#include "uiodviewer2dposgrp.h"

namespace FlatView { class Appearance; }
class uiBitMapDisplay;
class MapDataPack;



mExpClass(uiBasemap) uiBasemapZSliceObject : public uiBaseMapObject
{
public:
				uiBasemapZSliceObject();
				~uiBasemapZSliceObject();

    inline const char*		getType() const     { return "ZSlice"; }
    void			setZSlice(const Viewer2DPosDataSel&);

    virtual void		update();

protected:

    void			colTabChgCB(CallBacker*);

    FlatView::Appearance&	appearance_;
    uiBitMapDisplay&		bitmapdisp_;
    MapDataPack*		dp_;
};



mExpClass(uiBasemap) uiBasemapZSliceParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapZSliceParentTreeItem(int id)
			    : uiBasemapParentTreeItem("Z Slice",id)	{}
protected:

};



mExpClass(uiBasemap) uiBasemapZSliceTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapZSliceTreeItem(const char*);
			~uiBasemapZSliceTreeItem();
    bool		usePar(const IOPar&);

protected:
    void		checkCB(CallBacker*);
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const;

    uiBasemapZSliceObject*	uibmobj_;

};



mExpClass(uiBasemap) uiBasemapZSliceGroup : public uiBasemapGroup
{
public:
			uiBasemapZSliceGroup(uiParent*,bool isadd);
			~uiBasemapZSliceGroup();

    virtual bool	acceptOK();
    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
    void		selChg(CallBacker*);
    virtual uiObject*	lastObject();

    uiODViewer2DPosGrp* posgrp_;
    uiGenInput*		zslicefld_;
};



mExpClass(uiBasemap) uiBasemapZSliceItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapZSliceItem,
				"Z Slice",
				sFactoryKeyword())

    int			defaultZValue() const;
    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif

