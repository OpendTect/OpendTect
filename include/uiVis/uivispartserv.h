#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.8 2002-04-11 06:41:00 kristofer Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
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
    static const int	evSelectionChange;
			//!< 
    static const int	evPicksChanged;
			//!<

    enum ElementType    { Inline, Crossline, Timeslice };
    enum ObjectType	{ Unknown, DataDisplay, PickSetDisplay };

    int			addScene();
    visSurvey::Scene*	getSelScene();

    void		turnOn(int,bool);
    bool		isOn(int);

    ObjectType		getObjectType( int id ) const;

    bool		deleteAllObjects();
    int			addDataDisplay(uiVisPartServer::ElementType);
    void		removeDataDisplay();

    int                 addPickSetDisplay();
    void                removePickSetDisplay();
    bool		setPicks(const PickSet&);
    void		getPickSets(UserIDSet&);
    void		getPickSetData(const char*,PickSet&);
    int 		nrPicks(int);

    void		setSelObjectId(int id);
    int			getSelObjectId() const;
    void		setSelSceneId(int id)	{ selsceneid = id; }
    int			getSelSceneId()		{ return selsceneid; }

    void		setColorSeq(const ColorTable&);
    const ColorTable&   getColorSeq();

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

protected:

    void		selectObjCB(CallBacker*);
    void		picksChangedCB(CallBacker*);
    void		showPosCB(CallBacker*);

    const CallBack	appcb;
    int			selsceneid;
    float		planepos;

    ObjectSet<visSurvey::PickSetDisplay>	picks;
    ObjectSet<visSurvey::SeisDisplay>		seisdisps;
    ObjectSet<visSurvey::Scene>         	scenes;
};

#endif
