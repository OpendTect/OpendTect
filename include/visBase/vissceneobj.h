#ifndef vissceneobj_h
#define vissceneobj_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vissceneobj.h,v 1.12 2003-11-07 12:21:54 bert Exp $
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
