#ifndef visdataman_h
#define visdataman_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdataman.h,v 1.9 2002-05-02 14:15:16 kristofer Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "callback.h"

class IOPar;

class SoPath;

namespace visBase
{
class DataObject;
class SelectionManager;
class Factory;


/*!\brief


*/

class DataManager : public CallBacker
{
public:
    			DataManager();
    virtual		~DataManager();

    bool		reInit();
    			/*!< Should be when everything should be removed
			     and reinitialized.
			*/


    void		fillPar( IOPar&, TypeSet<int> & ) const;
    bool		usePar( const IOPar& );

    void		ref( int id );
    void		ref( const DataObject* );
    void		unRef( int id );
    void		unRef( const DataObject* );

    int			getId( const DataObject* ) const;
    int			getId( const SoPath* ) const;
    void		getIds( const SoPath*, TypeSet<int>& ) const;
    			/*!< Gets the ids from lowest level to highest
			     (i.e. scene ) */
    void		getIds( const type_info&, TypeSet<int>& ) const;

    DataObject*		getObj( int id );
    const DataObject*	getObj( int id ) const;

    SelectionManager&	selMan() { return selman; }
    Factory&		factory() { return fact; }

    Notifier<DataManager>	removeallnotify;

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
    Factory&			fact;

    static const char*		freeidstr;
    static const char*		selmanprefix;
};

DataManager& DM();

};


#endif
