#ifndef visdataman_h
#define visdataman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdataman.h,v 1.22 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "factory.h"
#include "callback.h"

class IOPar;

class SoPath;
class SoNode;

namespace visBase
{
class DataObject;
class SelectionManager;
class Factory;


/*!\brief


*/

mClass DataManager : public CallBacker
{
public:
    			DataManager();
    virtual		~DataManager();

    static void		readLockDB();
    static void		readUnLockDB();
    static void		writeLockDB();
    static void		writeUnLockDB();

    bool		removeAll(int nriterations=1000);
    			/*!< Will remove everything.  */

    void		fillPar(IOPar&,TypeSet<int>&) const;
    bool		usePar(const IOPar&);

    void		getIds(const SoPath*,TypeSet<int>&) const;
    			/*!< Gets the ids from lowest level to highest
			     (i.e. scene ) */

    void		getIds(const std::type_info&,TypeSet<int>&) const;
    int			highestID() const;

    DataObject*		getObject(int id);
    const DataObject*	getObject(int id) const;
    DataObject*		getObject(const SoNode*);
    const DataObject*	getObject(const SoNode*) const;

    SelectionManager&	selMan() { return selman_; }

    Notifier<DataManager>	removeallnotify;

    mDefineFactoryInClass( DataObject, factory );
protected:

    friend class	DataObject;
    void		addObject( DataObject* );
    void		removeObject( DataObject* );

    ObjectSet<DataObject>	objects_;

    int				freeid_;
    SelectionManager&		selman_;

    static const char*		sKeyFreeID();
    static const char*		sKeySelManPrefix();
};

mGlobal DataManager& DM();

};


#endif
