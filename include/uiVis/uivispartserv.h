#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.14 2002-04-16 11:05:02 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
#include "thread.h"

class UserIDSet;
class PickSet;
class PickSetGroup;
class Color;
class ColorTable;
class CallBack;
class CubeSampling;
class AttribSelSpec;
class AttribSlice;

namespace visSurvey
{
class Scene;
class PickSetDisplay;
class SeisDisplay;
};

namespace visBase
{
    class Material;
    class ColorSequence;
};

namespace Threads { class Mutex; };


/*! \brief Service provider for application level - Visutes */

class uiVisPartServer : public uiApplPartServer
{
public:
			uiVisPartServer(uiApplService&);
			~uiVisPartServer();
    const char*		name() const		{ return "Visualisation"; }

    			//Events and their functions
    static const int	evShowPosition;
    static const int	evSelection;
    static const int	evDeSelection;
    static const int	evPicksChanged;
    static const int	evGetNewData;
    static const int	evSelectableStatusCh;
    int			getEventObjId() const { return eventobjid; }
    			/* Tells which object the event is about */

    			//General stuff
    bool		deleteAllObjects();

    enum ElementType    { Inline, Crossline, Timeslice };
    enum ObjectType	{ Unknown, DataDisplay, PickSetDisplay };
    ObjectType		getObjectType( int ) const;
    void		setObjectName(int,const char*);
    const char*		getObjectName(int);

    void		turnOn(int,bool);
    bool		isOn(int);
    void		setViewMode(bool yn);

    			// Selection
    bool		isSelectable(int) const;
    void		makeSelectable(int, bool yn );
    void		setSelObjectId(int);
    			//!< set to -1 if you want toedeselect
    int			getSelObjectId() const;

    			// Scene Stuff
    int			addScene();
    void		setSelSceneId(int id)	{ selsceneid = id; }
    int			getSelSceneId()		{ return selsceneid; }

			//DataDisplay stuff
    int			addDataDisplay(uiVisPartServer::ElementType);
    void		removeDataDisplay(int);
    void		resetManipulation( int );
    float		getPlanePos(int) const;
    void		setAttribSelSpec(int,AttribSelSpec&);
    CubeSampling&	getCubeSampling(int,bool manippos);
    AttribSelSpec&	getAttribSelSpec(int);
    void		putNewData(int,AttribSlice*);

    			//PickSets stuff
    int                 addPickSetDisplay();
    void                removePickSetDisplay(int);
    bool		setPicks(int, const PickSet&);
    void		getAllPickSets(UserIDSet&);
    void		getPickSetData(const char*,PickSet&);
    int 		nrPicks(int);

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

			//Dialogs
    bool		setZScale();
    void		setMaterial(int);

protected:

    void		selectObjCB(CallBacker*);
    void		deselectObjCB(CallBacker*);
    void		picksChangedCB(CallBacker*);
    void		showPosCB(CallBacker*);
    void		getDataCB(CallBacker*);

    const CallBack	appcb;
    int			selsceneid;
    bool		viewmode;

    Threads::Mutex&	eventmutex;
    int			eventobjid;

    ObjectSet<visSurvey::PickSetDisplay>	picks;
    ObjectSet<visSurvey::SeisDisplay>		seisdisps;
    ObjectSet<visSurvey::Scene>         	scenes;
};

#endif
