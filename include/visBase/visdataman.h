#ifndef visdataman_h
#define visdataman_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdataman.h,v 1.2 2002-04-08 07:23:32 kristofer Exp $
________________________________________________________________________


-*/

#include "sets.h"

class SoNode;

namespace visBase
{
class DataObject;
class SelectionManager;


/*!\brief


*/

class DataManager
{
public:
    			DataManager();
    virtual		~DataManager();

    bool		reInit();
    			/*!< Should be when everything should be removed
			     and reinitialized.
			*/


    void		ref( int id );
    void		ref( const DataObject* );
    void		unRef( int id );
    void		unRef( const DataObject* );

    int			getId( const DataObject* ) const;
    int			getId( const SoNode* ) const;

    DataObject*		getObj( int id );
    const DataObject*	getObj( int id ) const;

    SelectionManager&	selMan() { return selman; }

    static DataManager	manager;

protected:
    bool		removeAll(int nriterations=1000);
    			/*!< Will remove everything.  */

    friend		DataObject;

    int			addObj( DataObject* );
			/*!< Returns id. If it already exist in the db, the
			     id of the old one is returned.
			*/

    void		remove( int idx );
    int			getIdx( int id ) const;

    ObjectSet<DataObject>	objects;
    TypeSet<int>		ids;
    TypeSet<int>		refcounts;

    int				freeid;
    SelectionManager&		selman;
};

DataManager& DM();

};


#endif
