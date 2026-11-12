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
namespace ODVsg
{
    class Annotation;
    class EventCatcher;
    class EventInfo;
    class MarkerSet;
    class TextureChannels;
    class VisualObject;

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

    ODVsg::DataObjectGroup* vsgGroup()		{ return group_.ptr(); }

    int			size() const override;
    bool        isVSGScene() const override { return true; }
    void		addObject(ODVsg::DataObject*);
    void		insertObject(int idx,ODVsg::DataObject*);
    void		addUTMObject(ODVsg::VisualObject*);
    void		addInlCrlZObject(ODVsg::DataObject*);
    void		removeObject(int idx) override;
    ODVsg::DataObject* getVsgObject(int idx);
    const ODVsg::DataObject* getVsgObject(int idx) const;
    int			getFirstIdx(const VisID&) const override;
    int			getFirstIdx(const ODVsg::DataObject*) const;

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
    OD::Color		getDisplayedAnnotColor() const;
    OD::Color		getDisplayedCornerAnnotColor() const;
    void		setBackgroundGradient(bool);
    bool		isBackgroundGradient() const	{ return gradientbg_; }
    OD::Color		getBackgroundCornerColor() const;
    void		setBackgroundColor(const OD::Color&);
    OD::Color		getBackgroundColor() const	{ return bgcolor_; }
    void		setAnnotFont(const FontData&) override;
    const FontData&	getAnnotFont() const override;

    const ODVsg::Annotation* getAnnotation() const
			{ return annot_.ptr(); }

    void		setEventCatcher(ODVsg::EventCatcher*);
    ODVsg::EventCatcher* eventCatcher()	{ return events_.ptr(); }

    void		setInteractionEnabled(bool yn);
    bool		isInteractionEnabled() const	{ return interact_; }

    TrcKeyValue		getMousePos() const override;
    uiString		getMousePosString() const override;

    void		applyEvent(const ODVsg::EventInfo&);
			/*!<Status, then selection when interaction is on.
			    The viewer uses the two notifiers instead, so a
			    dragger can mark the event handled in between. */

    void		addInteractionTest();
    ODVsg::TextureChannels* testTextureChannels()
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
    static const char*	sKeyBackgroundColor();
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
    void		updateStatus(const ODVsg::EventInfo&);
    void		updateMarkerInteraction(const ODVsg::EventInfo&);
    int			markerUnderCursor(const ODVsg::EventInfo&) const;
    bool		surfaceUnderCursor(const Coord&,Coord3&);
    void		updateSelection(const ODVsg::EventInfo&);
    void		addSurfacePick(const Coord3& displaypos);
    bool		displayToInlCrl(const Coord3& displaypos,
					Coord3& objectpos) const;

    RefMan<ODVsg::DataObjectGroup> group_;
    RefMan<mDGBVisTrans>	tempzstretchtrans_;
    RefMan<mDGBVisTrans>	inlcrlrotation_;
    RefMan<mDGBVisTrans>	inlcrlscale_;
    RefMan<mDGBVisTrans>	utm2disptransform_;
    RefMan<ODVsg::Annotation>	annot_;
    RefMan<ODVsg::EventCatcher> events_;
    RefMan<ODVsg::MarkerSet>	picks_;
    RefMan<ODVsg::TextureChannels> testchannels_;
    int				draggingmarker_		= -1;
    Coord3			xytmousepos_		= Coord3::udf();
    TrcKey			mousetrckey_		= TrcKey::udf();
    uiString			mouseposstr_;
    OD::Color			annotsavedcolor_	= OD::Color::White();
    OD::Color			annotcolor_	= OD::Color::White();
    OD::Color			bgcolor_	= OD::Color::Anthracite();
    bool			gradientbg_	= false;
    bool			interact_		= false;
    bool			testsceneadded_		= false;

    TrcKeyZSampling		tkzs_;
    TrcKeyZSampling		inittkzs_;
    TrcKeyZSampling		annotscale_;
    const ZDomain::Info*	zdomaininfo_;
    float			zscale_;
    float			curzstretch_ = 2.f;
};


mGlobal(vsgSurvey) void initSurveyClasses();

} // namespace ODVsg
