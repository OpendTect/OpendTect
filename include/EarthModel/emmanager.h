#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.29 2006-04-28 15:19:52 cvsnanne Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "ptrman.h"
#include "multiid.h"
#include "emposid.h"

class IOObj;
class IOObjContext;
class Executor;

namespace EM
{
class ObjectFactory;
class EMObject;
class History;
class SurfaceIOData;
class SurfaceIODataSelection;

/*!\brief


*/

class EMManager
{
public:
			EMManager();
			~EMManager();

    void		init();
    void		empty();

    History&		history();
    const History&	history() const;

    int			nrLoadedObjects() const	{ return objects.size(); }
    EM::ObjectID	objectID(int idx) const;
    Executor*		objectLoader(const MultiID&,
	    			     const SurfaceIODataSelection* =0);
    Executor*		objectLoader(const TypeSet<MultiID>&,
	    			     const SurfaceIODataSelection* =0);
    EM::ObjectID	createObject(const char* type,const char* name);
    			/*!< Creates a new object, saves it and loads it into
			     mem.
			     \note If an object already exist with that name,
			     it will be removed!! Check in advance with
			     findObject().
			*/
    MultiID		findObject( const char* type, const char* name ) const;
    void		sendRemovalSignal(const ObjectID&);
    BufferString	objectName(const MultiID&) const;
    			/*!<\returns the name of the object */
    const char*		objectType(const MultiID&) const;
    			/*!<\returns the type of the object */

    EMObject*		getObject(const ObjectID&);
    const EMObject*	getObject(const ObjectID&) const;

    EMObject*		createTempObject(const char* type);

    const char*		getSurfaceData(const MultiID&, SurfaceIOData&);
    			// returns err msg or null if OK

    void		addFactory( ObjectFactory* fact );

    			/*Interface from EMObject to report themselves */
    ObjectID		addObject(EMObject*);
    void		removeObject(EMObject*);

    const IOObjContext*	getContext( const char* type ) const;
    ObjectID		getObjectID( const MultiID& ) const;
    			/*!<\note that the relationship between storage id 
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */
    MultiID		getMultiID( const ObjectID& ) const;
    			/*!<\note that the relationship between storage id 
			     (MultiID) and EarthModel id (ObjectID) may change
			     due to "Save as" operations and similar. */

protected:
    const ObjectFactory*	getFactory( const char* type ) const;
    ObjectSet<ObjectFactory>	objectfactories;

    History&			history_;
    int				freeid;

    ObjectSet<EMObject>		objects;
    BufferString		errmsg;
};


EMManager& EMM();

}; // Namespace


#endif
