#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.6 2003-01-03 15:50:15 nanne Exp $
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
    const char*		getName( const MultiID& );

    MultiID		add( Type, const char* name );
    			/*! Return the id */

    EMObject*		getObject( const MultiID& );
    const EMObject*	getObject( const MultiID& ) const;
    void		removeObject(const MultiID&);

    Executor*		load( const MultiID& );
    bool		isLoaded( const MultiID& ) const;

protected:
    ObjectSet<EMObject>	objects;
    BufferString	errmsg;
};


EMManager& EMM();

}; // Namespace


#endif
