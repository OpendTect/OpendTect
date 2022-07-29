#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2009
________________________________________________________________________

-*/

#include "uivismod.h"
#include "dpsdispmgr.h"
#include "emposid.h"
#include "menuhandler.h"
#include "thread.h"

class DataPointSet;
class uiVisPartServer;
namespace visSurvey { class PointSetDisplay; }


/*!
\brief Implementation of DataPointSetDisplayMgr that enables display of
datapointsets in 3d visualization scenes.
*/

mExpClass(uiVis) uiVisDataPointSetDisplayMgr : public DataPointSetDisplayMgr
{ mODTextTranslationClass(uiVisDataPointSetDisplayMgr);
public:
		uiVisDataPointSetDisplayMgr(uiVisPartServer&);
		~uiVisDataPointSetDisplayMgr();

    void	lock();
    void	unLock();

    int		getNrViewers() const;
		//!<Will return the number of scenes
    const char*	getViewerName(int) const;

    bool	hasDisplays() const
		{ return displayinfos_.size()>0; }
    DispID	getDisplayID(const DataPointSet&) const;
    DispID	getDisplayID(int visid) const;

    DispID	addDisplay(const TypeSet<int>& parents, const DataPointSet&);
    bool	addDisplays(const TypeSet<int>& parents,
			    const ObjectSet<DataPointSet>&,TypeSet<DispID>&);
    void	updateDisplay(DispID id, const TypeSet<int>& parents,
			      const DataPointSet&);
    void	turnOn(DispID id,bool);
    void	updateDisplay(DispID id,const DataPointSet&);
    void	updateColorsOnly(DispID id);
    void	removeDisplay(DispID);
    void	clearDisplays();

    void	getIconInfo(BufferString& fnm,BufferString& tooltip) const;

    CNotifier<uiVisDataPointSetDisplayMgr,EM::ObjectID>	treeToBeAdded;
    mClass(uiVis) DisplayInfo
    {
    public:
	TypeSet<int>		sceneids_;
	TypeSet<int>		visids_; //linked with scenes_
    };

protected:

    TypeSet<int>		allsceneids_;

    MenuItem			createbodymnuitem_;
    MenuItem			storepsmnuitem_;
    MenuItem			removemnuitem_;
    MenuItem			propmenuitem_;
    TypeSet<int>		ids_;
    ObjectSet<DisplayInfo>	displayinfos_; //linked with ids_

    uiVisPartServer&		visserv_;
    MenuHandler*		vismenu_;
    Threads::Mutex		lock_;

    void	createMenuCB(CallBacker*);
    void	handleMenuCB(CallBacker*);
    void	removeDisplayAtIndex(int);
};

