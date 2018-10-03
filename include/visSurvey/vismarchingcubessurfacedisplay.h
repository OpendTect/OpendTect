#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "vissurveycommon.h"
#include "attribsel.h"
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
				    SurveyObject,MarchingCubesDisplay,
				     "MarchingCubesDisplay",
				     ::toUiString(sFactoryKeyword()) );
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    DBKey			getDBKey() const;
    bool			isInlCrl() const	{ return true; }

    bool			hasColor() const	{ return true; }
    bool			usesColor() const;
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

    void			useTexture(bool yn);
    bool			usesTexture() const;
    bool			showsTexture() const;
    bool			canShowTexture() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    bool			setVisSurface(visBase::MarchingCubesSurface*);
					//!<Creates an EM::Object for it.
    bool			setEMID(const DBKey&,TaskRunner*);
    DBKey			getEMID() const;

    const uiString&		errMsg() const { return errmsg_; }

    SurveyObject::AttribFormat	getAttributeFormat(int) const;
    int				nrAttribs() const;
    bool			canAddAttrib(int) const;
    bool			canRemoveAttrib() const;
    bool			canHandleColTabSeqTrans(int) const;
    const ColTab::Mapper&	getColTabMapper(int) const;
    void			setColTabMapper(int,const ColTab::Mapper&,
						TaskRunner*);
    const ColTab::Sequence&	getColTabSequence(int) const;
    bool			canSetColTabSequence() const;
    void			setColTabSequence(int,const ColTab::Sequence&,
					TaskRunner*);
    void			setSelSpecs(int attrib,
					    const Attrib::SelSpecList&);
    const Attrib::SelSpec*	getSelSpec(int attrib,int version=0) const;
    const Attrib::SelSpecList*	getSelSpecs(int attrib) const;
    void			setDepthAsAttrib(int);
    void			setIsoPatch(int);
    void			enableAttrib(int attrib,bool yn);
    bool			isAttribEnabled(int attrib) const;
    bool			hasSingleColorFallback() const	{ return true; }

    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			setRandomPosData(int,const DataPointSet*,
						 const TaskRunnerProvider&);

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

    visBase::MarchingCubesSurface*		displaysurface_;
    EM::MarchingCubesSurface*			emsurface_;
    Attrib::SelSpecList				as_;
    ObjectSet<DataPointSet>			cache_;

    bool					usestexture_;
    bool					validtexture_;
    bool					isattribenabled_;

    EM::ImplicitBody*				impbody_;
    bool					displayintersections_;

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
    visBase::Transformation*			model2displayspacetransform_;
    const mVisTrans*				intersectiontransform_;
};

} // namespace visSurvey
