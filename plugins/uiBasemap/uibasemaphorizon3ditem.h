#ifndef uibasemaphorizon3ditem_h
#define uibasemaphorizon3ditem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"
#include "uibasemap.h"

namespace EM { class Horizon3D; }
namespace FlatView { class Appearance; }
class uiBitMapDisplay;
class MapDataPack;


mExpClass(uiBasemap) uiBasemapHorizon3DGroup : public uiBasemapGroup
{
public:
			uiBasemapHorizon3DGroup(uiParent*,bool isadd);
			~uiBasemapHorizon3DGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    virtual uiObject*	lastObject();
    void		selChg(CallBacker*);

    uiIOObjSelGrp*	ioobjfld_;
    TypeSet<MultiID>	mids_;
};


mExpClass(uiBasemap) uiBasemapHorizon3DObject : public uiBaseMapObject
{
public:
			uiBasemapHorizon3DObject();
			~uiBasemapHorizon3DObject();

    inline const char*	getType() const			{ return "Horizon3D"; }
    void		setHorizon(EM::Horizon3D*);
    EM::Horizon3D*	getHorizon() const;

    virtual void	update();

protected:

    void			colTabChgCB(CallBacker*);

    FlatView::Appearance&	appearance_;
    uiBitMapDisplay&		bitmapdisp_;
    EM::Horizon3D*		hor3d_;
    MapDataPack*		dp_;
};



mExpClass(uiBasemap) uiBasemapHorizon3DParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapHorizon3DParentTreeItem(int id)
			    : uiBasemapParentTreeItem("Horizon 3D",id)	{}

protected:
    const char*		iconName() const;
};



mExpClass(uiBasemap) uiBasemapHorizon3DTreeItem : public uiBasemapTreeItem
{ mODTextTranslationClass(uiBasemapHorizon3DTreeItem)
public:
			uiBasemapHorizon3DTreeItem(const char*);
			~uiBasemapHorizon3DTreeItem();
    bool		usePar(const IOPar&);

protected:
    void		checkCB(CallBacker*);
    void		show(bool);
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const;

    uiBasemapHorizon3DObject*	uibmobj_;
};


mExpClass(uiBasemap) uiBasemapHorizon3DItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapHorizon3DItem,
				"Horizon 3D",
				sFactoryKeyword())

    int			defaultZValue() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
