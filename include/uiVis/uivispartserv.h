#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.70 2002-12-17 11:14:52 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
#include "sets.h"
#include "thread.h"
#include "position.h"

class UserIDSet;
class PickSet;
class PickSetGroup;
class Color;
class ColorTable;
class CallBack;
class CubeSampling;
class AttribSelSpec;
class AttribSlice;
class AttribSliceSet;
class IOPar;
class MultiID;
class BinIDRange;
class SurfaceInfo;

namespace visSurvey
{
class Scene;
class PickSetDisplay;
class WellDisplay;
class SurfaceDisplay;
class PlaneDataDisplay;
class VolumeDisplay;
class SurfaceInterpreterDisplay;
};

namespace visBase
{
    class Material;
    class ColorSequence;
};

namespace Threads { class Mutex; };
namespace Geometry { class GridSurface; };


/*! \brief Service provider for application level - Visutes */

class uiVisPartServer : public uiApplPartServer
{
public:
			uiVisPartServer(uiApplService&);
			~uiVisPartServer();
    const char*		name() const		{ return "Visualisation"; }

    			//Events and their functions
    static const int	evManipulatorMove;
    static const int	evSelection;
    static const int	evDeSelection;
    static const int	evPicksChanged;
    static const int	evGetNewData;
    static const int	evSelectableStatusCh;
    static const int	evMouseMove;
    int			getEventObjId() const { return eventobjid; }
    			/* Tells which object the event is about */

    			//General stuff
    bool		deleteAllObjects();

    bool		usePar( const IOPar& );
    void		fillPar( IOPar& ) const;

    enum ElementType    { Inline, Crossline, Timeslice };
    enum ObjectType	{ Unknown, DataDisplay, PickSetDisplay, WellDisplay,
    			  HorizonDisplay, FaultDisplay, VolumeDisplay };
    ObjectType		getObjectType( int ) const;
    void		setObjectName(int,const char*);
    const char*		getObjectName(int) const;

    bool		dumpOI( int id, const char* filename ) const;
    			/*!< Writes out all display data to a file. This is
			     only for debugging COIN stuff. Should not be used
			     apart from that
			*/

    			/* Mouse stuff */
    Coord3		getMousePos(bool xyt) const
    			{ return xyt ? xytmousepos : inlcrlmousepos; }
    			/*!< If !xyt mouse pos will be in inl, crl, t */
    float		getMousePosVal() const { return mouseposval; }

    void		turnOn(int,bool);
    bool		isOn(int);
    void		setViewMode(bool yn);
    void		showAnnotText(int,bool);
    bool		isAnnotTextShown(int);
    void		showAnnotScale(int,bool);
    bool		isAnnotScaleShown(int);
    void		showAnnot(int,bool);
    bool		isAnnotShown(int);

    			// Selection
    bool		isSelectable(int) const;
    void		makeSelectable(int, bool yn );
    void		setSelObjectId(int);
    			//!< set to -1 if you want toedeselect
    int			getSelObjectId() const;

    			// Scene Stuff
    int			addScene();
    void		removeScene(int);
    void		setSelSceneId(int id)	{ selsceneid = id; }
    int			getSelSceneId()		{ return selsceneid; }
    void		getSceneIds(TypeSet<int>&) const;

			//DataDisplay stuff
    int			addDataDisplay(uiVisPartServer::ElementType);
    int			duplicateDisplay(int);
    void		removeDataDisplay(int);
    void		resetManipulation( int );
    void		setPlanePos(int);
    void		updatePlanePos(CallBacker*);
    float		getPlanePos(int) const;
    bool		isPlaneManipulated(int) const;
    void		setAttribSelSpec(int,AttribSelSpec&);
    CubeSampling&	getPrevCubeSampling(int);
    CubeSampling&	getCubeSampling(int,bool manippos);
    AttribSelSpec&	getAttribSelSpec(int);
    void		putNewData(int,AttribSlice*);
    AttribSlice*	getPrevData(int);
    void		getDataDisplayIds(int,uiVisPartServer::ElementType,
					  TypeSet<int>&);

			//VolumeDisplay stuff
    int			addVolumeDisplay();
    void		removeVolumeDisplay(int);
    void		putNewVolData(int,AttribSliceSet*);
    AttribSliceSet*	getPrevVolData(int);
    void		getVolumeDisplayIds(int,TypeSet<int>&);
    void		getVolumePlaneIds(int,int&,int&,int&);
    float		getVolumePlanePos(int,int dim) const;
    bool		isVolumeManipulated(int) const;
    int			addVolRen(int);

    			//PickSets stuff
    int                 addPickSetDisplay();
    void                removePickSetDisplay(int);
    bool		setPicks(int, const PickSet&);
    void		getAllPickSets(UserIDSet&);
    void		getPickSetData(const char*,PickSet&);
    void		getPickSetIds(int,TypeSet<int>&);
    int 		nrPicks(int);
    bool		picksetIsPresent(const char*);
    void		showAllPicks(int,bool);
    bool		allPicksShown(int);
    int			getPickType(int) const;
    void		setPickType(int,int);

    			//Well stuff
    int			addWellDisplay(const MultiID& emwellid);
    void		removeWellDisplay(int);
    int			getNrWellAttribs(int) const;
    const char*		getWellAttribName(int, int idx) const;
    void		displayWellAttrib(int, int idx);
    int			displayedWellAttrib(int) const;
    void		modifyLineStyle(int);
    void		showWellText(int,bool);
    bool		isWellTextShown(int) const;
    void		getWellIds(int,TypeSet<int>&);

    			// Surface stuff
    int			addSurfaceDisplay(const MultiID& emhorid);
    void		removeSurfaceDisplay( int );
    void		getSurfAttribData(int,
	    				 ObjectSet< TypeSet<BinIDZValue> >&,
	    				 bool posonly,
					 const BinIDRange* br=0) const;
    			//!< The data in the objset will be managed by caller
    void		getSurfAttribValues(int,TypeSet<float>&) const;
    void		putNewSurfData(int,
	    			      const ObjectSet< TypeSet<BinIDZValue> >&);
    void		getSurfaceIds(int,ObjectType,TypeSet<int>&) const;
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&,ObjectType) 
									const;
    void		getSurfaceNames(ObjectSet<BufferString>&,ObjectType) 
									const;
    void		setSurfaceNrTriPerPixel(int id, float);
    float		getSurfaceNrTriPerPixel(int id) const;

			// Resolution
    void		setResolution(int, int res );
    			//!< 0 is automatic.
    int			getResolution(int) const;
    int			getNrResolutions(int) const;
    BufferString	getResolutionText(int,int) const;

			//Interpretation stuff
    void		showSurfTrackerCube(bool yn, int sceneid=-1);
    bool		isSurfTrackerCubeShown(int sceneid=-1) const;
    int			getSurfTrackerCube(bool create=false);
    CubeSampling	surfTrackerCubeSampling();

    int			addSurfTracker( Geometry::GridSurface& );
    void		removeSurfTracker( int surf );

			//ColorSeqs
    bool		canSetColorSeq(int) const;
    void		modifyColorSeq(int, const ColorTable&);
    const ColorTable&   getColorSeq(int) const;
    void		shareColorSeq(int toid, int fromid );

    			//Ranges
    bool		canSetRange(int) const;
    void		setClipRate(int,float);
    float		getClipRate(int) const;
    void		setAutoscale(int,bool);
    bool		getAutoscale(int) const;
    void		setDataRange(int,const Interval<float>&);
    Interval<float>	getDataRange(int) const;
    void		shareRange(int toid, int fromid );
    			//!<Includes colorsequence

    			//Material
    bool		canSetColor(int) const;
    void		modifyColor( int, const Color&);
    Color		getColor(int) const;
    void		shareColor(int toid, int fromid );
    void		useTexture(int,bool);
    bool		usesTexture(int) const;

			//Dialogs
    bool		setWorkingArea();
    bool		setZScale();
    void		setMaterial(int);
    void		setPickProps(int);

protected:

    void		selectObjCB(CallBacker*);
    void		deselectObjCB(CallBacker*);
    void		picksChangedCB(CallBacker*);
    void		manipMoveCB(CallBacker*);
    void		getDataCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);

    const CallBack	appcb;
    int			selsceneid;
    bool		viewmode;

    Threads::Mutex&	eventmutex;
    int			eventobjid;
    int			cbobjid;

    Coord3		xytmousepos;
    Coord3		inlcrlmousepos;
    float		mouseposval;

    ObjectSet<visSurvey::PickSetDisplay>	picks;
    ObjectSet<visSurvey::PlaneDataDisplay>	seisdisps;
    ObjectSet<visSurvey::WellDisplay>         	wells;
    ObjectSet<visSurvey::SurfaceDisplay>       	surfaces;
    ObjectSet<visSurvey::Scene>         	scenes;
    ObjectSet<visSurvey::VolumeDisplay>		volumes;
    visSurvey::SurfaceInterpreterDisplay*	surftracker;
    
    
    static const char*	workareastr;
    static const char*	appvelstr;
};

#endif
