#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.7 2003-02-03 14:10:34 nanne Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "ptrman.h"
#include "multiid.h"

class Executor;

namespace EarthModel
{
class EMObject;

/*!\brief


*/

class EMManager
{
public:
    enum Type		{ Hor, Well, Fault };


			EMManager();
			~EMManager();

    void		init();
    const char*		getName(const MultiID&);

    MultiID		add(Type,const char* name);
    			/*! Return the id */

    int			nrObjects() const	{ return objects.size(); }
    
    EMObject*		getObject(const MultiID&);
    const EMObject*	getObject(const MultiID&) const;
    const EMObject*	getEMObject(int) const;
    EMObject*		getEMObject(int);
    void		removeObject(const MultiID&);

    Executor*		load(const MultiID&);
    bool		isLoaded(const MultiID&) const;

protected:
    ObjectSet<EMObject>	objects;
    BufferString	errmsg;
};


EMManager& EMM();

}; // Namespace


#endif
