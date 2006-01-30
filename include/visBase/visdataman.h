#ifndef visdataman_h
#define visdataman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdataman.h,v 1.16 2006-01-30 14:45:34 cvskris Exp $
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
    int			highestID() const;

    DataObject*		getObject( int id );
    const DataObject*	getObject( int id ) const;

    SelectionManager&	selMan() { return selman_; }
    Factory&		factory() { return fact_; }

    Notifier<DataManager>	removeallnotify;

protected:

    friend class	DataObject;
    void		addObject( DataObject* );
    void		removeObject( DataObject* );

    ObjectSet<DataObject>	objects_;

    int				freeid_;
    SelectionManager&		selman_;
    Factory&			fact_;

    static const char*		sKeyFreeID();
    static const char*		sKeySelManPrefix();
};

DataManager& DM();

};


#endif
