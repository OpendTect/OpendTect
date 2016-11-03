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

mExpClass(uiODMain) uiODSceneProbeParentTreeItem
			: public uiODSceneParentTreeItem
{   mODTextTranslationClass(uiODSceneProbeParentTreeItem);
public:

    enum Type		{ Empty, Select, Default, RGBA };

			uiODSceneProbeParentTreeItem(const uiString&);
    const char*		childObjTypeKey() const;

    bool		showSubMenu();
    virtual bool	canShowSubMenu() const		{ return true; }
    virtual bool	canAddFromWell() const		{ return true ; }
    virtual Probe*	createNewProbe() const		=0;

    static uiString	sAddEmptyPlane();
    static uiString	sAddAndSelectData();
    static uiString	sAddDefaultData();
    static uiString	sAddColorBlended();
    static uiString	sAddAtWellLocation();

protected:

    bool		fillProbe(Probe&,Type);
    bool		setDefaultAttribLayer(Probe&) const;
    bool		setSelAttribProbeLayer(Probe&) const;
    bool		setRGBProbeLayers(Probe&) const;
};


mExpClass(uiODMain) uiODSceneProbeTreeItem : public uiODDisplayTreeItem
{   mODTextTranslationClass(uiODSceneProbeTreeItem);
public:
			~uiODSceneProbeTreeItem();
protected:
			uiODSceneProbeTreeItem(Probe&);
    virtual bool	init();
    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const
			{ return 0; } //TODO PrIMPL remove later, temporary

    const Probe*	getProbe() const;
    Probe*		getProbe();
};
