#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.18 2003-12-12 10:50:08 kristofer Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "ptrman.h"
#include "multiid.h"
#include "emposid.h"

class Executor;

namespace EM
{
class EMObject;
class History;
class SurfaceIOData;
class SurfaceIODataSelection;

/*!\brief


*/

class EMManager
{
public:
    enum Type		{ Hor, Fault, StickSet };

			EMManager();
			~EMManager();

    History&		history();
    const History&	history() const;

    void		init();

    EM::ObjectID	add(Type,const char* name);
    			/*!< Creates a new object, saves it and loads it into
			     mem
			*/

    int			nrObjects() const	{ return objects.size(); }
    const EMObject*	getEMObject(int index) const { return objects[index]; }
    EMObject*		getEMObject(int index) { return objects[index]; }

    EMObject*		getObject(const EM::ObjectID&);
    const EMObject*	getObject(const EM::ObjectID&) const;

    Executor*		load(const MultiID&,
	    		     const EM::SurfaceIODataSelection* =0);
    EMObject*		getTempObj(EM::EMManager::Type);

    void		getSurfaceData(const MultiID&, EM::SurfaceIOData&);

    void		ref(const EM::ObjectID&);
    void		unRef(const EM::ObjectID&);
    void		unRefNoDel(const EM::ObjectID&);

    static EM::ObjectID	multiID2ObjectID( const MultiID& );

protected:
    void		removeObject(const EM::ObjectID&);
    History&		history_;
    ObjectSet<EMObject>	objects;
    TypeSet<int>	refcounts;
    BufferString	errmsg;
};


EMManager& EMM();

}; // Namespace


#endif
