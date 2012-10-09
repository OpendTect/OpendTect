#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "callback.h"
#include "factory.h"
#include "position.h"
#include "ptrman.h"
#include "ranges.h"
#include "multiid.h"
#include "emposid.h"

class Undo;
class IOObj;
class IOObjContext;
class TaskRunner;
class Executor;
class uiEMPartServer;

template <class T> class Selector;

namespace EM
{
class EMObject;
class SurfaceIOData;
class SurfaceIODataSelection;

/*!\brief Manages the loaded/half loaded EM objects in OpendTect. */


mClass EMManager : public CallBacker
{
public:
			EMManager();
			~EMManager();

    void		empty();

    Undo&		undo();
    const Undo&		undo() const;

    bool		objectExists( const EMObject* obj ) const;

    int			nrLoadedObjects() const	{ return objects_.size(); }
    inline int		size() const		{ return nrLoadedObjects(); }
    EM::ObjectID	objectID(int idx) const;
    Executor*		objectLoader(const MultiID&,
	    			     const SurfaceIODataSelection* =0);
    Executor*		objectLoader(const TypeSet<MultiID>&,
	    			     const SurfaceIODataSelection* =0);
    EMObject*		loadIfNotFullyLoaded(const MultiID&,TaskRunner* =0);
			/*!<If fully loaded, the loaded instance
			    will be returned. Otherwise, it will be loaded.
			    Returned object must be reffed by caller
			    (and eventually unreffed). */
    EM::ObjectID	createObject(const char* type,const char* name);
    			/*!< Creates a new object, saves it and loads it.
			Removes any loaded object with the same name!  */

    Notifier<EMManager>				addRemove;

    BufferString	objectName(const MultiID&) const;
    			/*!<\returns the name of the object */
    const char*		objectType(const MultiID&) const;
    			/*!<\returns the type of the object */

    EMObject*		getObject(const ObjectID&);
    const EMObject*	getObject(const ObjectID&) const;

    EMObject*		createTempObject(const char* type);

    const char*		getSurfaceData(const MultiID&,SurfaceIOData&);
    			//!<\returns err msg or null if OK

    			/*Interface from EMObject to report themselves */
    void		addObject(EMObject*);
    void		removeObject(const EMObject*);

    ObjectID		getObjectID(const MultiID&) const;
    			/*!<\note that the relationship between storage id 
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */
    MultiID		getMultiID(const ObjectID&) const;
    			/*!<\note that the relationship between storage id 
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */

    void		burstAlertToAll(bool yn);

    bool		sortHorizonsList(const TypeSet<MultiID>&,
	    				 TypeSet<MultiID>&,bool is2d) const;

    void		removeSelected(const ObjectID&,const Selector<Coord3>&,
	    			       TaskRunner*);
    
    IOPar*		getSurfacePars(const IOObj&) const;

    bool		readPars(const MultiID&,IOPar&) const;
    bool		writePars(const MultiID&,const IOPar&) const;

    void		levelToBeRemoved(CallBacker*);

protected:
    Undo&			undo_;

    ObjectSet<EMObject>		objects_;
};


mDefineFactory1Param( EMObject, EMManager&, EMOF );

mGlobal EMManager& EMM();

}; // Namespace


#endif
