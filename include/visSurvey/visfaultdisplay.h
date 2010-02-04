#ifndef visfaultdisplay_h
#define visfaultdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visfaultdisplay.h,v 1.22 2010-02-04 17:20:24 cvsjaap Exp $
________________________________________________________________________


-*/

#include "vismultiattribsurvobj.h"

#include "emposid.h"
#include "ranges.h"


class DataPointSet;

namespace visBase
{
    class GeomIndexedShape;
    class Transformation;
    class PickStyle;
    class ShapeHints;
    class IndexedPolyLine3D;
};

namespace EM { class Fault3D; }
namespace MPE { class FaultEditor; }
namespace Geometry { class ExplFaultStickSurface; class ExplPlaneIntersection; }


namespace visSurvey
{
class MPEEditor;

/*!\brief 


*/

mClass FaultDisplay : public MultiTextureSurveyObject
{
public:
    static FaultDisplay*	create()
				mCreateDataObj(FaultDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    virtual int			nrResolutions() const;
    virtual void		setResolution(int,TaskRunner*);

    SurveyObject::AttribFormat	getAttributeFormat(int) const
				{ return SurveyObject::RandomPos; }
    void			getRandomPos(DataPointSet&,TaskRunner*) const;
    void			setRandomPosData(int,const DataPointSet*,
	    					 TaskRunner*); 

    bool			hasColor() const		{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const	{ return true; }
    NotifierAccess*		materialChange();

    void			useTexture( bool yn, bool trigger );
    bool			usesTexture() const;
    bool			showingTexture() const;
    void			setDepthAsAttrib(int);

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void			setRightHandSystem(bool);

    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			display(bool sticks,bool panels);
    bool			areSticksDisplayed() const;
    bool			arePanelsDisplayed() const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);
    bool			canRemoveSelecion()		{ return true; }

    void			displayIntersections(bool yn);
    bool			areIntersectionsDisplayed() const;

    Notifier<FaultDisplay>	colorchange;

    void			updateKnotMarkers();

    void			setStickSelectMode(bool yn);
    bool			isInStickSelectMode() const;

protected:

    virtual			~FaultDisplay();
    void			otherObjectsMoved(
	    			    const ObjectSet<const SurveyObject>&,
				    int whichobj);
    void			setRandomPosDataInternal(int attrib,
	    						 const DataPointSet*,
							 int column,
							 TaskRunner*); 

    void			updateStickDisplay();
    void			updateSingleColor();
    void			updateManipulator();

    virtual bool		getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    virtual void		addCache();
    virtual void		removeCache(int);
    virtual void		swapCache(int,int);
    virtual void		emptyCache(int);
    virtual bool		hasCache(int) const;

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);
    void			stickSelectCB(CallBacker*);
    void			polygonFinishedCB(CallBacker*);

    void			setActiveStick(const EM::PosID&);
    void 			updateActiveStickMarker();

    visBase::EventCatcher*		eventcatcher_;
    visBase::Transformation*		displaytransform_;
    visBase::ShapeHints*		shapehints_;

    visBase::GeomIndexedShape*		paneldisplay_;
    Geometry::ExplFaultStickSurface*	explicitpanels_;

    visBase::GeomIndexedShape*		stickdisplay_;
    Geometry::ExplFaultStickSurface*	explicitsticks_;

    visBase::GeomIndexedShape*		intersectiondisplay_;
    Geometry::ExplPlaneIntersection*	explicitintersections_;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    visBase::PickStyle*			activestickmarkerpickstyle_;
    visBase::IndexedPolyLine3D*		activestickmarker_;
    int					activestick_;

    EM::Fault3D*			emfault_;
    MPE::FaultEditor*			faulteditor_;
    visSurvey::MPEEditor*		viseditor_;

    Coord3				mousepos_;

    TypeSet<DataPack::ID>		datapackids_;
    bool				showmanipulator_;

    bool				validtexture_;
    Color				nontexturecol_;
    bool				usestexture_;

    bool				displaysticks_;

    bool				stickselectmode_;
    bool				ctrldown_;
    ObjectSet<visBase::DataObjectGroup>	knotmarkers_;
};

};


#endif
