#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgsurveymod.h"

#include "visvsgdatagroup.h"
#include "vissurvscene.h"

#include "color.h"
#include "coord.h"
#include "fontdata.h"
#include "trckeyvalue.h"
#include "trckeyzsampling.h"

namespace ZDomain { class Info; }
namespace dGBVis
{
    class Annotation;
    class EventCatcher;
    class EventInfo;
    class MarkerSet;
    class TextureChannels;
    class VisualObject;
}

namespace dGBVisSurvey
{

/*!\brief Counterpart of visSurvey::Scene: the transform chain, Z stretch,
  and the survey box annotation.

  This is the only place where the scene elements are bound to the survey:
  the Annotation gets SI().sampling(true) and the work area from here.

  Scene graph, as in visSurvey::Scene:
    tempzstretchtrans_			UTM objects (display transformation
					utm2disptransform_)
      inlcrlrotation_			inl/crl/Z objects, among which the
					annotation (display transformation
					inlcrlscale_)
*/

mExpClass(vsgSurvey) Scene : public visSurvey::Scene
{
public:
    static RefMan<Scene> create();

    dGBVis::DataObjectGroup* vsgGroup()		{ return group_.ptr(); }

    int			size() const override;
    void		addObject(dGBVis::DataObject*);
    void		insertObject(int idx,dGBVis::DataObject*);
    void		addUTMObject(dGBVis::VisualObject*);
    void		addInlCrlZObject(dGBVis::DataObject*);
    void		removeObject(int idx) override;
    dGBVis::DataObject* getVsgObject(int idx);
    const dGBVis::DataObject* getVsgObject(int idx) const;
    int			getFirstIdx(const VisID&) const override;
    int			getFirstIdx(const dGBVis::DataObject*) const;

    void		setTrcKeyZSampling(const TrcKeyZSampling&) override;
    void		setTrcKeyZSampling(const TrcKeyZSampling&,
					   bool workarea) override;
    const TrcKeyZSampling& getTrcKeyZSampling(bool workarea=true) const
								override;

    void		setAnnotScale(const TrcKeyZSampling&,
				      const double* scalefacs=nullptr,
				      int nrvals=0) override;
    const TrcKeyZSampling& getAnnotScale(bool getdefault=true) const override;

    void		setFixedZStretch(float) override;
    float		getFixedZStretch() const override;
    void		setTempZStretch(float) override;
    float		getTempZStretch() const override;
    void		setZScale(float) override;
    float		getZScale() const override;
    float		getApparentVelocity(float zstretch) const override;

    const mDGBVisTrans* getTempZStretchTransform() const;
    const mDGBVisTrans* getInlCrl2DisplayTransform() const;
    const mDGBVisTrans* getUTM2DisplayTransform() const;

    bool		isRightHandSystem() const override;

    void		setZDomainInfo(const ZDomain::Info&) override;
    const ZDomain::Info& zDomainInfo() const override;
    uiString		zDomainUserName() const override;

    void		showAnnotText(bool yn) override;
    bool		isAnnotTextShown() const override;
    void		showAnnotScale(bool yn) override;
    bool		isAnnotScaleShown() const override;
    void		showAnnotGrid(bool yn) override;
    bool		isAnnotGridShown() const override;
    void		showAnnot(bool yn) override;
    bool		isAnnotShown() const override;
    void		setAnnotText(int dim,const uiString&) override;
    void		setAnnotColor(const OD::Color&) override;
    const OD::Color	getAnnotColor() const override;
    void		setAnnotFont(const FontData&) override;
    const FontData&	getAnnotFont() const override;

    const dGBVis::Annotation* getAnnotation() const
			{ return annot_.ptr(); }

    void		setEventCatcher(dGBVis::EventCatcher*);
    dGBVis::EventCatcher* eventCatcher()	{ return events_.ptr(); }

    void		setInteractionEnabled(bool yn);
    bool		isInteractionEnabled() const	{ return interact_; }

    TrcKeyValue		getMousePos() const override;
    uiString		getMousePosString() const override;

    void		applyEvent(const dGBVis::EventInfo&);
			/*!<Status, then selection when interaction is on.
			    The viewer uses the two notifiers instead, so a
			    dragger can mark the event handled in between. */

    void		addInteractionTest();
    dGBVis::TextureChannels* testTextureChannels()
			{ return testchannels_.ptr(); }
			//!<The data of the colored plane of the test that was
			//!<grabbed last, if they were added

    Notifier<Scene>	testchannelschanged;
    Notifier<Scene>	sceneboundingboxupdated;
    Notifier<Scene>	mouseposchange;

    static const char*	sKeyShowAnnot();
    static const char*	sKeyShowScale();
    static const char*	sKeyShowGrid();
    static const char*	sKeyAnnotColor();
    static const char*	sKeyZStretch();

private:
			Scene();
			~Scene();

    void		setup();
    void		updateTransforms(const TrcKeyZSampling&);
    void		updateAnnotationText();
    void		bindDraggers();
    void		mouseCB(CallBacker*);
    void		draggerStartedCB(CallBacker*);
    void		notHandledCB(CallBacker*);
    void		annotSelCB(CallBacker*);
    void		annotDeSelCB(CallBacker*);
    void		updateStatus(const dGBVis::EventInfo&);
    void		updateMarkerInteraction(const dGBVis::EventInfo&);
    int			markerUnderCursor(const dGBVis::EventInfo&) const;
    bool		surfaceUnderCursor(const Coord&,Coord3&);
    void		updateSelection(const dGBVis::EventInfo&);
    void		addSurfacePick(const Coord3& displaypos);
    bool		displayToInlCrl(const Coord3& displaypos,
					Coord3& objectpos) const;

    RefMan<dGBVis::DataObjectGroup> group_;
    RefMan<mDGBVisTrans>	tempzstretchtrans_;
    RefMan<mDGBVisTrans>	inlcrlrotation_;
    RefMan<mDGBVisTrans>	inlcrlscale_;
    RefMan<mDGBVisTrans>	utm2disptransform_;
    RefMan<dGBVis::Annotation>	annot_;
    RefMan<dGBVis::EventCatcher> events_;
    RefMan<dGBVis::MarkerSet>	picks_;
    RefMan<dGBVis::TextureChannels> testchannels_;
    int				draggingmarker_		= -1;
    Coord3			xytmousepos_		= Coord3::udf();
    TrcKey			mousetrckey_		= TrcKey::udf();
    uiString			mouseposstr_;
    OD::Color			annotsavedcolor_	= OD::Color::White();
    bool			interact_		= false;
    bool			testsceneadded_		= false;

    TrcKeyZSampling		tkzs_;
    TrcKeyZSampling		inittkzs_;
    TrcKeyZSampling		annotscale_;
    const ZDomain::Info*	zdomaininfo_;
    float			zscale_;
    float			curzstretch_ = 2.f;
};


mGlobal(vsgSurvey) void initStdClasses();

} // namespace dGBVisSurvey
