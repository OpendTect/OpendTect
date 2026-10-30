#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgsurveymod.h"

#include "visvsgdatagroup.h"

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

mExpClass(vsgSurvey) Scene : public dGBVis::DataObjectGroup
{
public:
    static RefMan<Scene> create();
			mCreateDGBVisObj(Scene);

    using dGBVis::DataObjectGroup::addObject;
    using dGBVis::DataObjectGroup::getObject;

    int			size() const override;
    void		addObject(dGBVis::DataObject*) override;
    void		insertObject(int idx,dGBVis::DataObject*) override;
    void		addUTMObject(dGBVis::VisualObject*);
    void		addInlCrlZObject(dGBVis::DataObject*);
    void		removeObject(int idx) override;
    dGBVis::DataObject* getObject(int idx) override;
    int			getFirstIdx(const VisID&) const override;
    int			getFirstIdx(const dGBVis::DataObject*) const override;

    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void		setTrcKeyZSampling(const TrcKeyZSampling&,
					   bool workarea);
    const TrcKeyZSampling& getTrcKeyZSampling(bool workarea=true) const;

    void		setAnnotScale(const TrcKeyZSampling&,
				      const double* scalefacs=nullptr,
				      int nrvals=0);
    const TrcKeyZSampling& getAnnotScale(bool getdefault=true) const;

    void		setFixedZStretch(float);
    float		getFixedZStretch() const;
    void		setTempZStretch(float);
    float		getTempZStretch() const;
    void		setZScale(float);
    float		getZScale() const;
    float		getApparentVelocity(float zstretch) const;

    const mDGBVisTrans* getTempZStretchTransform() const;
    const mDGBVisTrans* getInlCrl2DisplayTransform() const;
    const mDGBVisTrans* getUTM2DisplayTransform() const;

    bool		isRightHandSystem() const override;

    void		setZDomainInfo(const ZDomain::Info&);
    const ZDomain::Info& zDomainInfo() const;
    uiString		zDomainUserName() const;

    void		showAnnotText(bool yn);
    bool		isAnnotTextShown() const;
    void		showAnnotScale(bool yn);
    bool		isAnnotScaleShown() const;
    void		showAnnotGrid(bool yn);
    bool		isAnnotGridShown() const;
    void		showAnnot(bool yn);
    bool		isAnnotShown() const;
    void		setAnnotText(int dim,const uiString&);
    void		setAnnotColor(const OD::Color&);
    const OD::Color	getAnnotColor() const;
    void		setAnnotFont(const FontData&);
    const FontData&	getAnnotFont() const;

    const dGBVis::Annotation* getAnnotation() const
			{ return annot_.ptr(); }

    void		setEventCatcher(dGBVis::EventCatcher*);
    dGBVis::EventCatcher* eventCatcher()	{ return events_.ptr(); }

    void		setInteractionEnabled(bool yn);
    bool		isInteractionEnabled() const	{ return interact_; }

    TrcKeyValue		getMousePos() const;
    uiString		getMousePosString() const;

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
