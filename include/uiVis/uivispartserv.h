#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.92 2003-09-22 13:13:35 kristofer Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
#include "sets.h"
#include "thread.h"
#include "position.h"

#include "cubesampling.h"
#include "attribsel.h"

class SeisTrc;

class AttribSliceSet;
class PickSet;
class uiPopupMenu;
class ColorTable;
class UserIDSet;
class SurfaceInfo;

namespace visSurvey
{
class Scene;
class SurfaceInterpreterDisplay;
};

namespace Threads { class Mutex; };
namespace Geometry { class GridSurface; };


/*! \brief Service provider for application level - Visutes */

class uiVisPartServer : public uiApplPartServer
{
    friend class 		uiVisMenu;

public:
				uiVisPartServer(uiApplService&);
				~uiVisPartServer();
    const char*			name() const;
    void			setObjectName(int,const char*);
    const char*			getObjectName(int) const;

    int				addScene();
    void			removeScene(int);

    void			shareObject(int sceneid,int id);

    int				addInlCrlTsl(int scene,int type);
    int				addRandomLine(int scene);
    int				addVolView(int scene);
    int				addSurface(int scene,const MultiID&);
    void			getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    int				addWell(int scene,const MultiID&);

    int				addInterpreter( int scene );
    int				addSurfEditor(int scid, Geometry::GridSurface&);

    void			removeObject(int id,int sceneid);

    int				addPickSet(int scene,const PickSet&);
    void			getAllPickSets(UserIDSet&);
    void			setPickSetData(int,const PickSet&);
    void			getPickSetData(int,PickSet&) const;

    int				addStickSet(int scene, const MultiID&);
    MultiID			getMultiID(int) const;
	
    int				getSelObjectId() const;
    void			setSelObjectId(int);

    void			makeSubMenu(uiPopupMenu&,int scene,int id);
    				/*!< Gives menu ids from 1024 and above */
    bool			handleSubMenuSel(int mnu,int scene,int id);

    				//Events and their functions
    static const int		evUpdateTree;
    void			getChildIds(int id,TypeSet<int>&) const;
				/*!< Gets a scenes' children or a volumes' parts
				     If id==-1, it will give the ids of the
				     scenes */
    BufferString		getTreeInfo(int id) const;
    BufferString		getDisplayName(int) const;

    bool			isInlCrlTsl(int,int dim) const;
    				/*!< dim==0 : inline
				     dim==1 : crossline
				     dim==2 : tslc */
    bool			isVolView(int) const;
    bool			isRandomLine(int) const;
    bool			isHorizon(int) const;
    bool			isFault(int) const;
    bool			isStickSet(int) const;
    bool			isWell(int) const;
    bool			isPickSet(int) const;
    bool			isInterpreter(int) const;
    bool			hasAttrib(int) const;

    static const int		evSelection;
    				/*<! Get the id with getEventObjId() */

    static const int		evDeSelection;
    				/*<! Get the id with getEventObjId() */

    static const int		evGetNewData;
    				/*<! Get the id with getEventObjId() */
    				/*!< Get selSpec with getSelSpec */
    const CubeSampling*		getCubeSampling(int) const;
    				/*!< Should only be called as a direct 
				     reply to evGetNewCubeData */
    const AttribSliceSet*	getCachedData(int) const;
    bool			calculateAttrib(int id,bool newsel);
    bool			setCubeData(int,AttribSliceSet*,
	    				    bool colordata=false);
    				/*!< data becomes mine */
    void			setSliceIdx(int,int);

    void			getRandomPosDataPos(int,
				    ObjectSet<TypeSet<BinIDZValue> >&) const;
    				/*!< Content of objectset becomes callers */
    void			setRandomPosData(int,const ObjectSet
				   <const TypeSet<const BinIDZValue> >&);
    				/*!< The data should have exactly the same
				     structure as the positions given in
				     getRandomPosDataPos */

    void			getRandomTrackPositions(int id,
	    						TypeSet<BinID>&) const;
    const Interval<float>	getRandomTraceZRange(int id) const;
    void			setRandomTrackData(int,ObjectSet<SeisTrc>&,
	    					   bool colordata=false);
    				/*!< Traces become mine */

    static const int		evMouseMove;
    Coord3			getMousePos(bool xyt) const;
				/*!< If !xyt mouse pos will be in inl, crl, t */
    BufferString		getMousePosVal() const;


    static const int		evSelectAttrib;
    void			setSelSpec(const AttribSelSpec&);
    				/*!< Should only be called as a direct 
				     reply to evSelectAttrib */

    static const int		evSelectColorAttrib;
    static const int		evGetColorData;
    const ColorAttribSel*	getColorSelSpec(int) const;
    void			setColorSelSpec(const ColorAttribSel&);
    void			setColorSelSpec(int,const ColorAttribSel&);
    void			setColorData(int,AttribSliceSet*);

    static const int		evInteraction;
    				/*<! Get the id with getEventObjId() */
    BufferString		getInteractionMsg(int id) const;
    				/*!< Returns dragger position or
				     Nr positions in picksets */

    				// ColorTable stuff
    int				getColTabId(int) const;
    void			setClipRate(int,float);
    const TypeSet<float>*	getHistogram(int) const;

				//General stuff
    int				getEventObjId() const;
    const AttribSelSpec*	getSelSpec(int) const;
    void			setSelSpec(int,const AttribSelSpec&);
    bool			deleteAllObjects();
    void			setZScale();
    bool			setWorkingArea();
    void			setViewMode(bool yn);
    void			turnOn(int,bool);
    bool			isOn(int) const;

    bool			canDuplicate(int) const;
    int				duplicateObject(int id,int sceneid);
    				/*!< \returns id of new object */
    
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

protected:

    visSurvey::Scene*		getScene(int);
    const visSurvey::Scene*	getScene(int) const;

    bool			selectAttrib(int id);

    bool			hasColorAttrib(int) const;
    bool			selectColorAttrib(int);
    bool			calculateColorAttrib(int,bool);

    bool			isMovable(int id) const;
    bool			setPosition(int id);

    bool			isManipulated(int id) const;
    void			acceptManipulation(int id);
    bool			resetManipulation(int id);

    bool			hasMaterial(int id) const;
    bool			setMaterial(int id);

    bool			hasColor(int id) const;

    void			setUpConnections(int id);
    				/*!< Should set all cbs for the object */
    void			removeConnections(int id);

    bool			dumpOI(int id) const;
    void			toggleDraggers();

    ObjectSet<visSurvey::Scene>	scenes;
    visSurvey::SurfaceInterpreterDisplay* interpreterdisplay;

    ColorAttribSel		colorspec;
    AttribSelSpec		attribspec;
    				/* For temporary use when sending 
				   evSelectAttrib events */

    uiVisMenu*			vismenu;

    Coord3			xytmousepos;
    Coord3			inlcrlmousepos;
    float			mouseposval;

    bool			viewmode;
    Threads::Mutex&		eventmutex;
    int				eventobjid;

    void			selectObjCB(CallBacker*);
    void			deselectObjCB(CallBacker*);
    void			interactionCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			updatePlanePos(CallBacker*);


    static const char*		workareastr;
    static const char*		appvelstr;
};

#endif
