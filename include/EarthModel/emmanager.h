#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.16 2003-11-07 12:21:51 bert Exp $
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
class SurfaceIOData;

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
    const char*		getName(const MultiID&);

    MultiID		add(Type,const char* name);

    int			nrObjects() const	{ return objects.size(); }
    
    EMObject*		getObject(const MultiID&);
    const EMObject*	getObject(const MultiID&) const;
    const EMObject*	getEMObject(int) const;
    EMObject*		getEMObject(int);

    Executor*		load(const MultiID&);
    bool		isLoaded(const MultiID&) const;
    EMObject*		createObject(const MultiID&,bool addtoman=true);
    EMObject*		getTempObj(EM::EMManager::Type);

    void		addObject(EM::EMObject*);

    void		getSurfaceData(const MultiID&,EM::SurfaceIOData&);

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
