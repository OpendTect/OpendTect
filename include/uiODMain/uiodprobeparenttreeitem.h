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

class Probe;
class ProbeLayer;

mExpClass(uiODMain) uiODSceneProbeParentTreeItem
			: public uiODSceneParentTreeItem
{   mODTextTranslationClass(uiODSceneProbeParentTreeItem);
public:

    enum Type		{ Empty, Select, Default, RGBA };

			uiODSceneProbeParentTreeItem(const uiString&);
    const char*		childObjTypeKey() const;

    virtual bool	showSubMenu();
    virtual void	addMenuItems();
    virtual bool	handleSubMenu(int mnuid);
    virtual bool	canShowSubMenu() const			{ return true; }
    virtual Probe*	createNewProbe() const			=0;
    virtual bool	addChildProbe();

    virtual Type	getType(int mnuid) const;
    static uiString	sAddEmptyPlane();
    static uiString	sAddAndSelectData();
    static uiString	sAddDefaultData();
    static uiString	sAddColorBlended();
    static int		sAddDefaultDataMenuID()			{ return 0; }
    static int		sAddAndSelectDataMenuID()		{ return 1; }
    static int		sAddColorBlendedMenuID()		{ return 2; }

    static bool		addDefaultAttribLayer(uiODApplMgr&,Probe&);

    protected:

    bool		fillProbe(Probe&);
    virtual bool	setProbeToBeAddedParams(int mnuid)	{ return true;}
    virtual bool	setDefaultAttribLayer(Probe&) const;
    virtual bool	setSelAttribProbeLayer(Probe&) const;
    virtual bool	setRGBProbeLayers(Probe&) const;
    virtual bool	getSelAttrSelSpec(Probe&,Attrib::SelSpec&) const;

    Type		typetobeadded_;
    uiMenu*		menu_;
};


mExpClass(uiODMain) uiODSceneProbeTreeItem : public uiODDisplayTreeItem
{   mODTextTranslationClass(uiODSceneProbeTreeItem);
public:
			~uiODSceneProbeTreeItem();

    const Probe*	getProbe() const;
    Probe*		getProbe();
    void		handleAddAttrib();
    virtual uiString	createDisplayName() const;
    uiODDataTreeItem*	createProbeLayerItem(ProbeLayer&) const;
protected:
			uiODSceneProbeTreeItem(Probe&);
    virtual bool	init();
    virtual uiODDataTreeItem* createAttribItem(const Attrib::SelSpec*) const;
};
