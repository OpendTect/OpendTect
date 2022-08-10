#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2002
________________________________________________________________________

-*/

#include "uivismod.h"
#include "pickretriever.h"
#include "position.h"

namespace visSurvey { class Scene; }
class uiVisPartServer;

mExpClass(uiVis) uiVisPickRetriever : public PickRetriever
{
public:
    			uiVisPickRetriever(uiVisPartServer*);
    bool		enable(const TypeSet<SceneID>* allowedscenes);
    NotifierAccess*	finished()		{ return &finished_; }

    void		reset();
    bool		success() const		{ return status_==Success; }
    bool		waiting() const		{ return status_==Waiting; }
    const Coord3&	getPos() const		{ return pickedpos_; }
    int			getTrcNr() const	{ return pickedtrcnr_; }
    Pos::GeomID		getGeomID() const  	{ return pickedgeomid_; }
    SceneID		getSceneID() const	{ return pickedscene_; }
    const TypeSet<VisID>& getPickedObjIDs() const { return pickedobjids_; }

    void		addScene(visSurvey::Scene*);
    void		removeScene(visSurvey::Scene*);

    SceneID		unTransformedSceneID() const;
    const ZAxisTransform* getZAxisTransform() const;

protected:
				~uiVisPickRetriever();
    void			pickCB(CallBacker*);
    void			resetPickedPos();

    ObjectSet<visSurvey::Scene>	scenes_;
    TypeSet<SceneID>		allowedscenes_;
    TypeSet<VisID>		pickedobjids_;

    enum Status			{ Idle, Waiting, Failed, Success } status_;
    Coord3			pickedpos_;
    int				pickedtrcnr_;
    Pos::GeomID			pickedgeomid_;

    SceneID			pickedscene_;
    Notifier<uiVisPickRetriever> finished_;
    uiVisPartServer*		visserv_;
};

