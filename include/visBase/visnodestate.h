#ifndef visnodestate_h
#define visnodestate_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Oct 2012
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visosg.h"
#include "objectset.h"
#include "callback.h"

namespace osg { class StateSet; class StateAttribute; }

namespace visBase
{
    
/*!Baseclass for objects manipulating the osg::StateSet. */

mExpClass(visBase) NodeState : public CallBacker
{ mRefCountImpl(NodeState);
public:

    void			attachStateSet(osg::StateSet*);
    void			detachStateSet(osg::StateSet*);
    virtual void		setPixelDensity(float)			{}

protected:
				NodeState();
    
    template <class T> T*	addAttribute(T* a) { doAdd(a); return a; }
    
private:
    
    void			doAdd(osg::StateAttribute*);
    void			doRemove(osg::StateAttribute*);
    virtual void		applyAttribute(osg::StateSet*,
					       osg::StateAttribute*);
    
    ObjectSet<osg::StateAttribute>	attributes_;
    ObjectSet<osg::StateSet>		statesets_;
};

};

#endif

