#ifndef visdataman_h
#define visdataman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdataman.h,v 1.14 2005-02-04 14:31:34 kristofer Exp $
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

    bool		removeAll(int nriterations=1000);
    			/*!< Will remove everything.  */

    void		fillPar( IOPar&, TypeSet<int> & ) const;
    bool		usePar( const IOPar& );

    void		getIds( const SoPath*, TypeSet<int>& ) const;
    			/*!< Gets the ids from lowest level to highest
			     (i.e. scene ) */

    void		getIds( const std::type_info&, TypeSet<int>& ) const;

    DataObject*		getObject( int id );
    const DataObject*	getObject( int id ) const;

    SelectionManager&	selMan() { return selman; }
    Factory&		factory() { return fact; }

    Notifier<DataManager>	removeallnotify;

protected:

    friend class	DataObject;
    void		addObject( DataObject* );
    void		removeObject( DataObject* );

    ObjectSet<DataObject>	objects;

    int				freeid;
    SelectionManager&		selman;
    Factory&			fact;

    static const char*		freeidstr;
    static const char*		selmanprefix;
};

DataManager& DM();

};


#endif
