#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vissceneobj.h,v 1.2 2002-02-09 13:38:42 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"

class SoNode;
class SoGroup;

namespace visBase
{

/*! \brief
    The base class for all objects that are visual or modify the
    scene.
*/

class SceneObject
{
public:

    virtual		~SceneObject()		{}
    virtual SoNode*	getData()		= 0;

};


class SceneObjectGroup : public SceneObject
{
public:
				SceneObjectGroup(bool separate=true);
    virtual			~SceneObjectGroup();

    int				addObject( SceneObject* );
				//!< I'll take over you!!
    void			removeObject( int id )		{}
    void			removeAll();

    virtual SoNode*		getData();

protected:

    SoGroup*			root;
    ObjectSet<SceneObject>	objects;

};

};


#endif
