#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visscene.h,v 1.11 2004-01-05 09:43:47 kristofer Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "visdatagroup.h"

class SoEnvironment;

namespace visBase
{
    class SelectionManager;
    class EventCatcher;

/*!\brief
    Scene manages all DataObjects and has some managing
    functions such as the selection management and variables that should
    be common for the whole scene.
*/

class Scene : public DataObjectGroup
{
public:
    static Scene*	create()
			mCreateDataObj(Scene);

    void		setAmbientLight( float );
    float		ambientLight() const;

    SoNode*		getInventorNode();

protected:
    virtual		~Scene();

private:
    EventCatcher&	mouseevents;
    int			mousedownid;

    void		mousePickCB( CallBacker* );

    SoEnvironment*	environment;
    SoGroup*		selroot;
};

};


#endif
