#ifndef emmanager_h
#define emmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emmanager.h,v 1.1 2002-05-16 14:19:03 kristofer Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "ptrman.h"

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
    const char*		getName( int id );

    int			add( Type, const char* name );
    			/*! Return the id */

    EMObject*		getObject( int id );
    const EMObject*	getObject( int id ) const;

    int			load( int id );
    bool		isLoaded( int id );

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
