#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visscene.h,v 1.15 2006-06-14 17:09:43 cvskris Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "visdatagroup.h"

class SoEnvironment;

namespace visBase
{
    class SelectionManager;
    class EventCatcher;
    class PolygonOffset;

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

    void		addObject(DataObject*);
    void		insertObject(int idx,DataObject*);

    void		setAmbientLight(float);
    float		ambientLight() const;

    bool		blockMouseSelection(bool yn);
    			/*!<\returns previous status. */

    SoNode*		getInventorNode();

protected:
    virtual		~Scene();
    EventCatcher&	events_;

private:
    int			mousedownid_;

    void		mousePickCB(CallBacker*);

    SoEnvironment*	environment_;
    PolygonOffset*	polygonoffset_;
    SoGroup*		selroot_;
    bool		blockmousesel_;
};

};


#endif
