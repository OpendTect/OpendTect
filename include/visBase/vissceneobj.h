#ifndef vissceneobj_h
#define vissceneobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vissceneobj.h,v 1.7 2002-03-11 10:46:12 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "visdata.h"

class SoNode;

namespace visBase
{

/*! \brief
    The base class for all objects that are visual or modify the
    scene.
*/

class SceneObject : public DataObject
{
public:
    virtual SoNode*	getData()		= 0;
    const SoNode*	getData() const;

};

};


#endif
