#ifndef vissceneobj_h
#define vissceneobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vissceneobj.h,v 1.4 2002-02-26 19:58:50 kristofer Exp $
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

    virtual int			addObject( SceneObject* );
				//!< I'll take over you!!
    virtual void		removeObject( int id )		{}
    virtual void		removeAll();

    virtual SoNode*		getData();
protected:

    ObjectSet<SceneObject>	objects;
private:
    SoGroup*			root;

};

};


#endif
