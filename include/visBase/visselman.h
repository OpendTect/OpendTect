#ifndef visselman_h
#define visselman_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visselman.h,v 1.1 2002-02-26 17:54:40 kristofer Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "callback.h"

class SoSelection;
class SoNode;
class SoPath;

namespace visBase
{
class Scene;
class SceneObject;
class VisualObject;

/*!\brief
SelectionManager handles VisualObject that can be selected. If an object can be
selected, it has to register himself with regSelObject. At registration it has
to give two objects, first of all, he gives the object that outside users
will associate him with when they want to add their CB to detect his selection.

Secondly it has to give the SceneObject that actually is selected.
*/

class SelectionManager : public CallBacker
{
public:
		SelectionManager();
    virtual	~SelectionManager();

    void	notifySelection( const VisualObject& assoc, const CallBack& );
    void	deNotifySelection( const VisualObject& assoc, const CallBack& );
    void	notifyDeSelection( const VisualObject& assoc, const CallBack& );
    void	deNotifyDeSelection( const VisualObject& assoc,const CallBack&);

    void	regSelObject( const VisualObject& associated,
	    		      const SceneObject& detected );
    void	unRegSelObject( const VisualObject& associated,
	    			const SceneObject* =0 );
    		/*!< If second arg is zero, all objects connected to
		     the ass obje will be will be unregistered.
		*/
protected:
    int			indexOf( const VisualObject& ) const;
    SoSelection*	getNode() { return node; }
    friend		Scene;

    ObjectSet<const VisualObject>		assobjs;
    ObjectSet<const SceneObject>		detobjs;
    ObjectSet<Notifier<SelectionManager> >	selnotifiers;
    ObjectSet<Notifier<SelectionManager> >	deselnotifiers;

    SoSelection*		node;

    void		select( SoPath* );
    void		deSelect( SoPath* );

    static void		selectCB(void*, SoPath* );
    static void		deSelectCB(void*, SoPath* );
};

};

#endif
