#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.10 2003-06-03 12:46:12 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "ptrman.h"
#include "multiid.h"

class Executor;

namespace EM
{
class EMObject;
class History;

/*!\brief


*/

class EMManager
{
public:
    enum Type		{ Hor, Well, Fault };

			EMManager();
			~EMManager();

    History&		history();
    const History&	history() const;

    void		init();
    const char*		getName(const MultiID&);

    MultiID		add(Type,const char* name);

    int			nrObjects() const	{ return objects.size(); }
    
    EMObject*		getObject(const MultiID&);
    const EMObject*	getObject(const MultiID&) const;
    const EMObject*	getEMObject(int) const;
    EMObject*		getEMObject(int);

    Executor*		load(const MultiID&);
    bool		isLoaded(const MultiID&) const;

    void		ref(const MultiID&);
    void		unRef(const MultiID&);
    void		unRefNoDel(const MultiID&);

protected:
    void		removeObject(const MultiID&);
    History&		history_;
    ObjectSet<EMObject>	objects;
    TypeSet<int>	refcounts;
    BufferString	errmsg;
};


EMManager& EMM();

}; // Namespace


#endif
