#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vissceneobj.h,v 1.1 2002-02-06 22:30:19 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"

class SoNode;
class SoSeparator;

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
				SceneObjectGroup();
    virtual			~SceneObjectGroup();

    int				addObject( SceneObject* );
				//!< I'll take over you!!
    void			removeObject( int id )		{}
    void			removeAll();

    virtual SoNode*		getData();

protected:

    SoSeparator*		root;
    ObjectSet<SceneObject>	objects;

};

};


#endif
