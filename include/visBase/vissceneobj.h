#ifndef vissceneobj_h
#define vissceneobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
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

mClass(visBase) SceneObject : public DataObject
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

