#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Oct 2012
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
