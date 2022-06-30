#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "attribsel.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"


namespace Geometry { class ExplicitIndexedShape; }
namespace EM { class MarchingCubesSurface; class ImplicitBody; }
namespace visBase { class GeomIndexedShape; class MarchingCubesSurface;
		    class Transformation; };

namespace visSurvey
{

mExpClass(visSurvey) MarchingCubesDisplay : public visBase::VisualObjectImpl,
			      public visSurvey::SurveyObject
{ mODTextTranslationClass(MarchingCubesDisplay);
public:
			    MarchingCubesDisplay();
			    mDefaultFactoryInstantiation(
				visSurvey::SurveyObject,MarchingCubesDisplay,
				 "MarchingCubesDisplay",
				 ::toUiString(sFactoryKeyword()) );

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return true; }

    bool			hasColor() const	{ return true; }
    bool			usesColor() const;
    OD::Color			getColor() const;
    void			setColor(OD::Color);
    bool			allowMaterialEdit() const { return true; }

    void			useTexture(bool yn,bool trigger);
    bool			canShowTexture() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    bool			setVisSurface(visBase::MarchingCubesSurface*);
				//!<Creates an EMObject for it.
    bool			setEMID(const EM::ObjectID&,TaskRunner*);
    EM::ObjectID		getEMID() const;

    const char*			errMsg() const { return errmsg_.str(); }

    SurveyObject::AttribFormat	getAttributeFormat(int) const;
    int				nrAttribs() const;
    bool			canAddAttrib(int) const;
    bool			canRemoveAttrib() const;
    bool			canHandleColTabSeqTrans(int) const;
    const ColTab::MapperSetup*	getColTabMapperSetup(int,int) const;

    void			setColTabMapperSetup(int,
					const ColTab::MapperSetup&,TaskRunner*);
    const ColTab::Sequence*	getColTabSequence(int) const;
    bool			canSetColTabSequence() const;
    void			setColTabSequence(int,const ColTab::Sequence&,
					TaskRunner*);
    void			setSelSpec(int,const Attrib::SelSpec&);
    void			setSelSpecs(int attrib,
					const TypeSet<Attrib::SelSpec>&);
    const Attrib::SelSpec*	getSelSpec(int attrib,int version=0) const;
    const TypeSet<Attrib::SelSpec>* getSelSpecs(int attrib) const;
    void			setDepthAsAttrib(int);
    void			setIsoPatch(int);
    void			enableAttrib(int attrib,bool yn);
    bool			isAttribEnabled(int attrib) const;
    bool			hasSingleColorFallback() const	{ return true; }

    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			setRandomPosData( int attrib,
					const DataPointSet*, TaskRunner*);

    void			setOnlyAtSectionsDisplay(bool yn);
    bool			displayedOnlyAtSections() const;

    bool			canRemoveSelection() const	{ return true; }
    void			removeSelection(const Selector<Coord3>&,
						TaskRunner*);
    EM::MarchingCubesSurface*	getMCSurface() const { return emsurface_; }
    visBase::MarchingCubesSurface* getDisplaySurface() const
						{ return displaysurface_; }

protected:

    virtual			~MarchingCubesDisplay();
    bool			updateVisFromEM(bool onlyshape,TaskRunner*);
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);
    void			materialChangeCB(CallBacker*);

    void			getMousePosInfo(const visBase::EventInfo& ei,
					IOPar& iop ) const;
    void			getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos, BufferString& val,
					BufferString& info) const;
    void			otherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					int whichobj);
    void			updateIntersectionDisplay();
    void			updateSingleColor();

    static const char*	sKeyEarthModelID()	{ return "EM ID"; }
    static const char*	sKeyAttribSelSpec()	{ return "Attrib SelSpec"; }
    static const char*	sKeyColTabMapper()	{ return "Coltab mapper"; }
    static const char*	sKeyColTabSequence()	{ return "Coltab sequence"; }
    static const char*	sKeyUseTexture()	{ return "Use texture"; }

    visBase::MarchingCubesSurface*		displaysurface_	= nullptr;
    EM::MarchingCubesSurface*			emsurface_	= nullptr;
    Attrib::SelSpec				selspec_; // Not used
    ObjectSet<DataPointSet>			cache_;
    TypeSet<Attrib::SelSpec>			selspecs_;

    bool					isattribenabled_ = true;

    EM::ImplicitBody*				impbody_	= nullptr;
    bool					displayintersections_ = false;

    struct PlaneIntersectInfo
    {
						PlaneIntersectInfo();
						~PlaneIntersectInfo();

	visBase::GeomIndexedShape*		visshape_;
	Geometry::ExplicitIndexedShape*		shape_;

	int					planeid_;
	char					planeorientation_;
	float					planepos_;

	bool					computed_;
    };

    ObjectSet<PlaneIntersectInfo>		intsinfo_;
    visBase::Transformation*			model2displayspacetransform_
								= nullptr;
    const mVisTrans*				intersectiontransform_ =nullptr;

public:
    void			displayIntersections(bool yn)
				{ setOnlyAtSectionsDisplay(yn); }
    bool			areIntersectionsDisplayed() const
				{ return displayedOnlyAtSections(); }
};

} // namespace visSurvey

