#ifndef vissceneobj_h
#define vissceneobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vissceneobj.h,v 1.11 2002-07-30 11:29:26 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "visdata.h"

class SoNode;

namespace visBase
{
class Transformation;


/*! \brief
    The base class for all objects that are visual or modify the
    scene.
*/

class SceneObject : public DataObject
{
public:
    virtual SoNode*	getData()		= 0;

    virtual int		usePar( const IOPar& iopar )
    			{ return DataObject::usePar(iopar); }
    virtual void	fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
    			{ DataObject::fillPar( iopar, saveids );}
    virtual void	setDisplayTransformation(const Transformation*);

protected:
				SceneObject();
				~SceneObject();

    const Transformation*	transformation;
};

};


#endif
