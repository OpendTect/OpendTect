#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    void	lock() override;
    void	unLock() override;

    int		getNrViewers() const override;
		//!<Will return the number of scenes
    const char* getViewerName(int) const override;

    bool	hasDisplays() const override
		{ return displayinfos_.size()>0; }
    DispID	getDisplayID(const DataPointSet&) const override;
    DispID	getDisplayID(const VisID&) const;

    DispID	addDisplay(const TypeSet<ParentID>&,
			   const DataPointSet&) override;
    bool	addDisplays(const TypeSet<ParentID>&,
			    const ObjectSet<DataPointSet>&,TypeSet<DispID>&);
    void	updateDisplay(const DispID&,const TypeSet<ParentID>&,
			      const DataPointSet&) override;
    void	turnOn(const DispID&,bool);
    void	updateDisplay(const DispID&,const DataPointSet&) override;
    void	updateColorsOnly(const DispID&);
    void	removeDisplay(const DispID&) override;
    void	clearDisplays();

    void	getIconInfo(BufferString& fnm,
			    BufferString& tooltip) const override;

    CNotifier<uiVisDataPointSetDisplayMgr,EM::ObjectID>	treeToBeAdded;
    mClass(uiVis) DisplayInfo
    {
    public:
				DisplayInfo(const DispID&);
				~DisplayInfo();

	TypeSet<SceneID>	sceneids_;
	TypeSet<VisID>		visids_; //linked with scenes_
	DispID			dispid_		= DispID::udf();
    };

protected:

    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
    void			removeDisplay(DisplayInfo&);
    const DisplayInfo*		getInfo(const DispID&) const;
    DisplayInfo*		getInfo(const DispID&);

    MenuItem			createbodymnuitem_;
    MenuItem			storepsmnuitem_;
    MenuItem			removemnuitem_;
    MenuItem			propmenuitem_;
    ObjectSet<DisplayInfo>	displayinfos_;
    TypeSet<SceneID>		allsceneids_;

    uiVisPartServer&		visserv_;
    RefMan<MenuHandler>		vismenu_;
    Threads::Mutex		lock_;

};
