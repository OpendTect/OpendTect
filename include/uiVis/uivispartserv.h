#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.5 2002-04-09 13:26:22 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
class UserIDSet;
class PickSet;
class PickSetGroup;
class Color;
class CallBack;
class CubeSampling;
class AttribSelSpec;
class AttribSlice;

namespace visSurvey
{
class Scene;
class PickSet;
class SeisDisplay;
};

namespace visBase{ class Material; };


/*! \brief Service provider for application level - Visutes */

class uiVisPartServer : public uiApplPartServer
{
public:
			uiVisPartServer(uiApplService&,const CallBack);
			~uiVisPartServer();
    const char*		name() const		{ return "Visualisation"; }

    static const int	evShowPosition;
			//!< Display position of selected plane.
    static const int	evSelectPlane;
			//!< 
    static const int	evDeselectPlane;
			//!<

    enum ElementType    { Inline, Crossline, Timeslice };

    int			addScene();
    visSurvey::Scene*	getSelScene();

    void		turnOn(int,bool);
    bool		isOn(int);

    bool		deleteAllObjects();
    int			addDataDisplay(uiVisPartServer::ElementType);
    void		removeDataDisplay();
    void		selectObj(CallBacker*);

    int                 addPickSetDisplay();
    void                removePickSetDisplay();
    bool		setPicks(const PickSet&);
    void		getPickSets(UserIDSet&);
    void		getPickSetData(const char*,PickSet&);
    int 		nrPicks(int);

    void		setSelObjectId(int id);
    int			getSelObjectId()	{ return selobjid; }
    void		setSelSceneId(int id)	{ selsceneid = id; }
    int			getSelSceneId()		{ return selsceneid; }

    void		setColor(const Color&);
    Color		getColor() const;
    visBase::Material*	getMaterial();

    void		setClipRate(float);
    float		getClipRate();
    void		setAutoscale(bool);
    bool		getAutoscale();
    void		setDataRange(const Interval<float>&);
    Interval<float>	getDataRange();

    CubeSampling&	getCubeSampling(bool);
    AttribSelSpec&	getAttribSelSpec();
    void		setAttribSelSpec(AttribSelSpec&);
    void		putNewData(AttribSlice*);
    float		getPlanePos();
    void		showPos(CallBacker*);

protected:

    const CallBack	appcb;
    int			selobjid;
    int			selsceneid;
    float		planepos;

    ObjectSet<visSurvey::PickSet>	picks;
    ObjectSet<visSurvey::SeisDisplay>	seisdisps;
    ObjectSet<visSurvey::Scene>         scenes;
};

#endif
