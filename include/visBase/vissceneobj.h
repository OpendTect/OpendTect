#ifndef vissceneobj_h
#define vissceneobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vissceneobj.h,v 1.3 2002-02-26 17:54:40 kristofer Exp $
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
    const SoNode*	getData() const;

};


class SceneObjectWrapper : public SceneObject
{
public:
    			SceneObjectWrapper( SoNode* n );

    virtual		~SceneObjectWrapper();

    SoNode*		getData() { return node; }

protected:
    SoNode*		node;

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

    ObjectSet<SceneObject>	objects;
private:
    SoGroup*			root;

};

};


#endif
