#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.22 2005-01-06 09:40:02 kristofer Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "ptrman.h"
#include "multiid.h"
#include "emposid.h"

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

    History&		history();
    const History&	history() const;

    int			nrLoadedObjects() const	{ return objects.size(); }
    EM::ObjectID	objectID(int idx) const;
    Executor*		loadObject(const MultiID&,
	    		     const EM::SurfaceIODataSelection* =0);
    EM::ObjectID	createObject(const char* type,const char* name);
    			/*!< Creates a new object, saves it and loads it into
			     mem
			     \note If an object already exist with that name,
			     it will be removed!! Check in advance with
			     searchForObject()
			*/
    BufferString	objectName(const EM::ObjectID&) const;
    			/*!<\returns the name of the object */
    const char*		objectType(const EM::ObjectID&) const;
    			/*!<\returns the type of the object */
    EM::ObjectID	findObject(const char* type, const char* name) const;
    			/*!<\returns the objectid if found, -1 otherwise */

    EMObject*		getObject(const EM::ObjectID&);
    const EMObject*	getObject(const EM::ObjectID&) const;

    EMObject*		createTempObject(const char* type);

    const char*		getSurfaceData(const MultiID&, EM::SurfaceIOData&);
    			// returns err msg or null if OK

    void		ref(const EM::ObjectID&);
    void		unRef(const EM::ObjectID&);
    void		unRefNoDel(const EM::ObjectID&);

    static EM::ObjectID	multiID2ObjectID( const MultiID& );

    void		addFactory( ObjectFactory* fact );

protected:
    ObjectSet<ObjectFactory>	objectfactories;

    const IOObjContext*	getContext( const char* type ) const;
    void		removeObject(const EM::ObjectID&);
    History&		history_;
    ObjectSet<EMObject>	objects;
    TypeSet<int>	refcounts;
    BufferString	errmsg;
};


EMManager& EMM();

}; // Namespace


#endif
