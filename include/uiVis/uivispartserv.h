#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.3 2002-03-29 17:26:52 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
class UserIDSet;
class PickSet;
class PickSetGroup;
class Color;

namespace visSurvey
{
class Scene;
class PickSet;
};


/*! \brief Service provider for application level - Visutes */

class uiVisPartServer : public uiApplPartServer
{
public:
			uiVisPartServer(uiApplService&);
			~uiVisPartServer();
    const char*		name() const		{ return "Visualisation"; }

    int			addScene();

    void		turnOn(int,bool);
    bool		isOn(int);

    int                 addPickSetDisplay();
    void                removePickSetDisplay();
    bool		setPicks(const PickSet&);
    void		getPickSets(UserIDSet&);
    void		getPickSetData(const char*,PickSet&);
    int 		nrPicks(int);

    void		setSelObjectId(int id)	{ selobjid = id; }
    void		setSelSceneId(int id)	{ selsceneid = id; }

    void		setColor(const Color&);
    Color		getColor();

protected:

    int			selobjid;
    int			selsceneid;

    ObjectSet<visSurvey::PickSet>	picks;
    ObjectSet<visSurvey::Scene>         scenes;
};

#endif
