#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.78 2003-01-30 12:53:38 kristofer Exp $
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
public:
				uiVisPartServer(uiApplService&);
				~uiVisPartServer();
    const char*			name() const;
    void			setObjectName(int,const char*);
    const char*			getObjectName(int) const;

    int				addScene();
    void			removeScene(int id);

    int				addInlCrlTsl(int scene,int type);
    int				addRandomLine(int scene);
    int				addVolView(int scene);
    int				addSurface(int scene, const MultiID& );
    int				addWell(int scene, const MultiID& emwellid );

    int				addSurfTrackerCube( int scene );
    				/*!< Get position with getCubeSampling */
    int				getSurfTrackerCubeId();
    int				addSurfTrackProposal(Geometry::GridSurface&);

    int				addPickSet(int scene, const PickSet& pickset );
    void			getAllPickSets(UserIDSet&);
    void			getPickSetData( int id, PickSet& pickset )const;

    MultiID			getMultiID( int id ) const;
	
    int				getSelObjectId() const;
    void			setSelObjectId(int);

    void			makeSubMenu( uiPopupMenu&, int scene, int id);
    				/*!< Gives menu ids from 1024 and above */
    bool			handleSubMenuSel( int mnu, int scene, int id);

    				//Events and their functions
    static const int		evUpdateTree;
    void			getChildIds( int id, TypeSet<int>& ) const;
				/*!< Gets a scenes' children or a volumes' parts
				     If id==-1, it will give the ids of the
				     scenes */
    BufferString		getTreeInfo(int id) const;
    BufferString		getAttribName(int id) const;
    bool			isInlCrlTsl(int,int dim) const;
    				/*!< dim==0 : inline
				     dim==1 : crossline
				     dim==2 : tslc */
    bool			isVolView(int) const;
    bool			isRandomLine(int) const;
    bool			isHorizon(int) const;
    bool			isFault(int) const;
    bool			isWell(int) const;
    bool			isPickSet(int) const;

    static const int		evSelection;
    				/*<! Get the id with getEventObjId() */

    static const int		evDeSelection;
    				/*<! Get the id with getEventObjId() */

    static const int		evGetNewCubeData;
    				/*<! Get the id with getEventObjId() */
    				/*!< Get selSpec with getSelSpec */
    const CubeSampling*		getCubeSampling( int id ) const;
    				/*!< Should only be called as a direct 
				     reply to evGetNewCubeData */
    const AttribSliceSet*	getCachedData( int id ) const;
    bool			setCubeData( int id, AttribSliceSet* );
    				/*!< data becomes mine */

    static const int		evGetNewRandomPosData;
    				/*!< Get selSpec with getSelSpec */
    				/*<! Get the id with getEventObjId() */
    void			getRandomPosDataPos( int id,
				    ObjectSet<TypeSet<BinIDZValue> >& ) const;
    				/*!< Content of objectset becomes callers */
    void			setRandomPosData( int id, const ObjectSet
				   <const TypeSet<const BinIDZValue> >&);
    				/*!< The data should have exactly the same
				     structure as the positions given in
				     getRandomPosDataPos */

    static const int		evGetRandomTracePosData;
    				/*<! Get the id with getEventObjId() */
    				/*!< Get selSpec with getSelSpec */
    void			getRandomTrackPositions(int id,
	    						TypeSet<BinID>&) const;
    const Interval<float>	getRandomTraceZRange( int id ) const;
    void			setRandomTrackData( int id,
	    						ObjectSet<SeisTrc>& );
    				/*!< Traces becomes mine */

    static const int		evMouseMove;
    Coord3			getMousePos(bool xyt) const;
				/*!< If !xyt mouse pos will be in inl, crl, t */
    BufferString		getMousePosVal() const;


    static const int		evSelectAttrib;
    void			setSelSpec(const AttribSelSpec&);
    				/*!< Should only be called as a direct 
				     reply to evSelectAttrib */

    static const int		evInteraction;
    				/*<! Get the id with getEventObjId() */
    BufferString		getInteractionMsg(int id) const;
    				/*!< Returns dragger position or
				     Nr positions in picksets */

    				// ColorTable stuff
    int				getColTabId(int) const;
    void			setClipRate(int,float);

				//General stuff
    int				getEventObjId() const;
    const AttribSelSpec*	getSelSpec( int id ) const;
    void			setSelSpec( int id, const AttribSelSpec& );
    bool			deleteAllObjects();
    void			setZScale();
    bool			setWorkingArea();
    void			setViewMode(bool yn);
    void			turnOn(int,bool);
    bool			isOn(int) const;

    bool			usePar( const IOPar& );
    void			fillPar( IOPar& ) const;

protected:

    visSurvey::Scene*		getScene( int id );
    const visSurvey::Scene*	getScene( int id ) const;
    void			removeObject( int id, int sceneid );

    bool			hasAttrib( int id ) const;
    bool			selectAttrib( int id );
    bool			calculateAttrib( int id, bool newsel );

    bool			isMovable( int id ) const;
    bool			setPosition( int id );

    bool			isManipulated( int id ) const;
    void			acceptManipulation( int id );
    bool			resetManipulation( int id );

    bool			hasMaterial( int id ) const;
    bool			setMaterial( int id );

    bool			hasColor( int id ) const;

    bool			hasDuplicate(int) const;
    bool			duplicateObject(int,int);
    
    void			setUpConnections( int id );
    				/*!< Should set all cbs for the object */
    bool			dumpOI( int id ) const;
    void			toggleDraggers();

    ObjectSet<visSurvey::Scene>	scenes;
    visSurvey::SurfaceInterpreterDisplay* interpreterdisplay;

    AttribSelSpec		attribspec;
    				/* For temporary use when sending 
				   evSelectAttrib events */

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
