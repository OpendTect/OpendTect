#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "sharedobject.h"

#include "objectset.h"
#include "visosg.h"

namespace osg { class StateSet; class StateAttribute; }

namespace visBase
{

/*!\brief Base class for objects manipulating the osg::StateSet. */

mExpClass(visBase) NodeState : public SharedObject
{
public:
    void			attachStateSet(osg::StateSet*);
    void			detachStateSet(osg::StateSet*);
    virtual void		setPixelDensity(float)			{}

protected:
				NodeState();
    virtual			~NodeState();

    template <class T> T*	addAttribute(T* a) { doAdd(a); return a; }
    template <class T> void	removeAttribute(T* a) {doRemove(a);}

private:

    void			doAdd(osg::StateAttribute*);
    void			doRemove(osg::StateAttribute*);
    virtual void		applyAttribute(osg::StateSet*,
					       osg::StateAttribute*);

    ObjectSet<osg::StateAttribute>	attributes_;
    ObjectSet<osg::StateSet>		statesets_;
};

} // namespace visBase
