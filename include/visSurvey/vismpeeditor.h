#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "draw.h"
#include "emeditor.h"
#include "emseedpicker.h"
#include "visdragger.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "visobject.h"
#include "vissower.h"
#include "vistransform.h"


namespace visSurvey
{

class Sower;

/*!\brief
*/


mExpClass(visSurvey) MPEEditor : public visBase::VisualObjectImpl
{
public:
    static RefMan<MPEEditor> create();
			mCreateDataObj(MPEEditor);

    void		setEditor(MPE::ObjectEditor*);
    void		setPatch(MPE::Patch*);
    RefMan<MPE::ObjectEditor> getMPEEditor();
    void		setSceneEventCatcher(visBase::EventCatcher*) override;

    void		setDisplayTransformation(const mVisTrans*) override;
    const mVisTrans*	getDisplayTransformation() const override;

    void		setMarkerSize(float);
    void		turnOnMarker(EM::PosID,bool yn);
    bool		allMarkersDisplayed() const;

    Notifier<MPEEditor>		nodeRightClick;
				/*!<\ the clicked position can be retrieved
				      with getNodePosID(getRightClickNode) */
    int				getRightClickNode() const;
    EM::PosID			getNodePosID(int idx) const;

    bool			mouseClick( const EM::PosID&, bool shift,
					    bool alt, bool ctrl );
				/*!<Notify the object that someone
				    has clicked on the object that's being
				    edited. Clicks on the editor's draggers
				    themselves are handled by clickCB.
				    Returns true when click is handled. */

    bool			clickCB( CallBacker* );
				/*!<Since the event should be handled
				    with this object before the main object,
				    the main object has to pass eventcatcher
				    calls here manually.
				    \returns wether the main object should
				    continue to process the event.
				*/
    EM::PosID			mouseClickDragger(const TypeSet<VisID>&) const;

    bool			isDragging() const	{ return isdragging_; }
    EM::PosID			getActiveDragger() const;
    Notifier<MPEEditor>		draggingStarted;

    Sower&			sower();
    const Sower&		sower() const;
    void			displayPatch(const MPE::Patch*);
    void			cleanPatch();
    const ObjectSet<visBase::MarkerSet>& getDraggerMarkers() const;
    void			setMarkerStyle(const MarkerStyle3D&);
    const MarkerStyle3D*	markerStyle() const;

protected:
				~MPEEditor();

    void			changeNumNodes(CallBacker*);
    void			nodeMovement(CallBacker*);
    void			dragStart(CallBacker*);
    void			dragMotion(CallBacker*);
    void			dragStop(CallBacker*);
    void			updateNodePos(int,const Coord3&);
    void			removeDragger(int);
    void			addDragger(const EM::PosID&);
    void			setActiveDragger(const EM::PosID&);
    void			setupPatchDisplay();

    int				rightclicknode_;

    WeakPtr<MPE::ObjectEditor>	emeditor_;
    RefMan<MPE::Patch>		patch_;

    RefMan<visBase::Material>	nodematerial_;
    RefMan<visBase::Material>	activenodematerial_;

    RefObjectSet<visBase::Dragger> draggers_;
    RefObjectSet<visBase::MarkerSet> draggermarkers_;
    TypeSet<EM::PosID>		posids_;
    float			markersize_ = 3.f;
    RefMan<visBase::MarkerSet>	patchmarkers_;
    RefMan<visBase::PolyLine>	patchline_;
    RefMan<visBase::EventCatcher> eventcatcher_;
    ConstRefMan<mVisTrans>	transformation_;
    EM::PosID			activedragger_;

    bool			draggerinmotion_ = false;
    bool			isdragging_ = false;

    RefMan<Sower>		sower_;
    MarkerStyle3D		markerstyle_;
};

} // namespace visSurvey
