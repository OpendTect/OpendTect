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

    enum AddType	{ DefaultData, DefaultAttrib, RGBA, Select, Empty };

			uiODSceneProbeParentTreeItem(const uiString&);
    const char*		childObjTypeKey() const;

    virtual bool	is2D() const			{ return false; }
    virtual bool	showSubMenu();
    virtual void	addMenuItems();
    virtual bool	handleSubMenu(int mnuid);
    virtual bool	canShowSubMenu() const		{ return true; }
    virtual Probe*	createNewProbe() const		= 0;
    virtual bool	addChildProbe();

    virtual AddType	getAddType(int mnuid) const;
    static uiString	sAddEmptyPlane();
    static uiString	sAddAndSelectData();
    static uiString	sAddDefaultData();
    static uiString	sAddDefaultAttrib();
    static uiString	sAddColorBlended();
    static int		cAddDefaultDataMenuID()		{ return DefaultData; }
    static int		cAddDefaultAttribMenuID()	{ return DefaultAttrib;}
    static int		cAddColorBlendedMenuID()	{ return RGBA; }
    static int		cAddAndSelectDataMenuID()	{ return Select; }

    static bool		addDefaultAttribLayer(uiODApplMgr&,Probe&,bool stored);

protected:

    bool		fillProbe(Probe&);
    virtual bool	setProbeToBeAddedParams(int mnuid)	{ return true;}
    virtual bool	setDefaultAttribLayer(Probe&,bool stored) const;
    virtual bool	setSelAttribProbeLayer(Probe&) const;
    virtual bool	setRGBProbeLayers(Probe&) const;
    virtual bool	getSelAttrSelSpec(Probe&,Attrib::SelSpec&) const;
    virtual bool	getSelRGBAttrSelSpecs(Probe&,
					      TypeSet<Attrib::SelSpec>&) const;

    AddType		typetobeadded_;
    uiMenu*		menu_;

};


mExpClass(uiODMain) uiODSceneProbeTreeItem : public uiODDisplayTreeItem
{   mODTextTranslationClass(uiODSceneProbeTreeItem);
public:
			~uiODSceneProbeTreeItem();

    const Probe*	getProbe() const;
    Probe*		getProbe();
    virtual PresInfo*	getObjPrInfo() const;
    void		handleAddAttrib();
    virtual uiString	createDisplayName() const;
    uiODDataTreeItem*	createProbeLayerItem(ProbeLayer&) const;

protected:

			uiODSceneProbeTreeItem(Probe&);
    virtual bool	init();
    virtual uiODDataTreeItem* createAttribItem(const Attrib::SelSpec*) const;

};
