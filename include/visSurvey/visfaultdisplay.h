#ifndef visfaultdisplay_h
#define visfaultdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visfaultdisplay.h,v 1.45 2012/05/21 14:09:42 cvsbruno Exp $
________________________________________________________________________


-*/

#include "vismultiattribsurvobj.h"

#include "emposid.h"
#include "ranges.h"
#include "explfaultsticksurface.h"

class DataPointSet;

namespace visBase
{
    class GeomIndexedShape;
    class Transformation;
    class PickStyle;
    class ShapeHints;
    class IndexedPolyLine3D;
    class DrawStyle;
};

namespace EM { class Fault3D; }
namespace MPE { class FaultEditor; }
namespace Geometry 
{ 
    class ExplPlaneIntersection;
    class FaultStickSurface; 
}


namespace visSurvey
{
class MPEEditor;
class HorizonDisplay;

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

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			setRightHandSystem(bool);

    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			triangulateAlg(mFltTriProj);
    mFltTriProj			triangulateAlg() const;

    void			display(bool sticks,bool panels);
    bool			areSticksDisplayed() const;
    bool			arePanelsDisplayed() const;
    bool			arePanelsDisplayedInFull() const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    void			removeSelection(const Selector<Coord3>&,
	    					TaskRunner*);
    bool			canRemoveSelection() const	{ return true; }

    void			displayIntersections(bool yn);
    bool			areIntersectionsDisplayed() const;
    bool			canDisplayIntersections() const;
    
    void			displayHorizonIntersections(bool yn); 
    bool			areHorizonIntersectionsDisplayed() const;
    bool			canDisplayHorizonIntersections() const;

    Notifier<FaultDisplay>	colorchange;
    Notifier<FaultDisplay>	displaymodechange;

    void			updateKnotMarkers();

    void			setStickSelectMode(bool yn);
    bool			isInStickSelectMode() const;

    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);
    void			getLineWidthBounds(int& min,int& max);

    virtual void		getMousePosInfo(const visBase::EventInfo& ei,
	    					IOPar& iop ) const
				{ return MultiTextureSurveyObject
				    	::getMousePosInfo(ei,iop); }
    void			getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					BufferString& info) const;


    bool			allowsPicks() const		{ return true; }
    bool			isVerticalPlane() const		{return false;}
    bool			canBDispOn2DViewer() const	{return false;}
    int				addDataPack(const DataPointSet&) const ;
    bool			setDataPackID(int attrib,DataPack::ID,
	    				      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPackMgr::ID		getDataPackMgrID() const
				{ return DataPackMgr::SurfID(); }

    static const char*		sKeyTriProjection() { return "TriangulateProj";}

protected:

    virtual			~FaultDisplay();
    void			otherObjectsMoved(
	    			    const ObjectSet<const SurveyObject>&,
				    int whichobj);
    void			setRandomPosDataInternal(int attrib,
	    						 const DataPointSet*,
							 int column,
							 TaskRunner*); 
    void			updatePanelDisplay();
    void			updateStickDisplay();
    void			updateIntersectionDisplay();
    void			updateHorizonIntersectionDisplay();
    void			updateDisplay();

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
    bool			isSelectableMarkerInPolySel(
					const Coord3& markerworldpos ) const;

    void			setActiveStick(const EM::PosID&);
    void 			updateActiveStickMarker();
    void			updateHorizonIntersections( int whichobj,
	    				const ObjectSet<const SurveyObject>&);
    void			updateEditorMarkers();

    Coord3			disp2world(const Coord3& displaypos) const;

    bool			coincidesWith2DLine(
					const Geometry::FaultStickSurface&,
					int sticknr) const;
    bool			coincidesWithPlane(
					const Geometry::FaultStickSurface&,
					int sticknr,
					TypeSet<Coord3>& intersectpoints) const;
    void			updateStickHiding();
    void			setLineRadius(visBase::GeomIndexedShape*);

    visBase::EventCatcher*		eventcatcher_;
    const mVisTrans*			displaytransform_;
    visBase::ShapeHints*		shapehints_;

    visBase::GeomIndexedShape*		paneldisplay_;
    Geometry::ExplFaultStickSurface*	explicitpanels_;

    visBase::GeomIndexedShape*		stickdisplay_;
    Geometry::ExplFaultStickSurface*	explicitsticks_;

    visBase::GeomIndexedShape*		intersectiondisplay_;
    Geometry::ExplPlaneIntersection*	explicitintersections_;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    ObjectSet<visBase::GeomIndexedShape> horintersections_;
    ObjectSet<Geometry::ExplFaultStickSurface>	horshapes_;
    TypeSet<int>			horintersectids_;
    bool				displayintersections_;
    bool				displayhorintersections_;
    
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
    bool				displaypanels_;

    bool				stickselectmode_;
    bool				ctrldown_;
    ObjectSet<visBase::DataObjectGroup>	knotmarkers_;

    struct StickIntersectPoint
    {
	Coord3				pos_;
	int				sid_;
	int				sticknr_;
    };

    ObjectSet<StickIntersectPoint> stickintersectpoints_;

    visBase::DrawStyle*			drawstyle_;

public:

    void                                doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>& objs,
				    int whichobj)
				    { otherObjectsMoved( objs, whichobj ); }

};

};


#endif
