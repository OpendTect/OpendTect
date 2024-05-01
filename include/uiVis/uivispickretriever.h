#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    bool		enable(const TypeSet<SceneID>* allowedscenes) override;
    NotifierAccess*	finished() override		{ return &finished_; }

    void		reset() override;
    bool		success() const override    { return status_==Success; }
    bool		waiting() const override    { return status_==Waiting; }
    const Coord3&	getPos() const override     { return pickedpos_; }
    int			getTrcNr() const override   { return pickedtrcnr_; }
    Pos::GeomID		getGeomID() const override  { return pickedgeomid_; }
    SceneID		getSceneID() const override { return pickedscene_; }
    const TypeSet<VisID>& getPickedObjIDs() const override
			{ return pickedobjids_; }

    void		addScene(visSurvey::Scene*);
    void		removeScene(visSurvey::Scene*);

    SceneID		unTransformedSceneID() const override;
    const ZAxisTransform* getZAxisTransform() const override;

protected:
				~uiVisPickRetriever();
    void			pickCB(CallBacker*);
    void			resetPickedPos();

    TypeSet<SceneID>		allowedscenes_;
    TypeSet<VisID>		pickedobjids_;

    enum Status			{ Idle, Waiting, Failed, Success };
    Status			status_		= Idle;
    Coord3			pickedpos_;
    int				pickedtrcnr_;
    Pos::GeomID			pickedgeomid_;

    SceneID			pickedscene_;
    Notifier<uiVisPickRetriever> finished_;
    uiVisPartServer*		visserv_;
};
