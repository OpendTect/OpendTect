#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.3 2002-05-22 07:00:08 kristofer Exp $
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
    enum Type		{ Hor, Well };


			EMManager();
			~EMManager();

    void		init();
    const char*		getName( const MultiID& );

    MultiID		add( Type, const char* name );
    			/*! Return the id */

    EMObject*		getObject( const MultiID& );
    const EMObject*	getObject( const MultiID& ) const;

    Executor*		load( const MultiID& );
    bool		isLoaded( const MultiID& ) const;

protected:
    ObjectSet<EMObject>	objects;
    BufferString	errmsg;
};


static inline EMManager& EMM()
{
    static PtrMan<EMManager> emm = 0;

    if ( !emm ) emm = new EarthModel::EMManager;
    return *emm;
}


}; // Namespace


#endif
