#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visscene.h,v 1.13 2005-04-05 12:30:45 cvskris Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "visdatagroup.h"

class SoEnvironment;
class SoPolygonOffset;

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

    void		addObject( DataObject* );
    void		insertObject( int idx, DataObject* );

    void		setAmbientLight( float );
    float		ambientLight() const;

    SoNode*		getInventorNode();

protected:
    virtual		~Scene();
    EventCatcher&	events;

private:
    int			mousedownid;

    void		mousePickCB( CallBacker* );

    SoEnvironment*	environment;
    SoPolygonOffset*	polygonoffset;
    SoGroup*		selroot;
};

};


#endif
