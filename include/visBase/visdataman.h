#ifndef visdataman_h
#define visdataman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdataman.h,v 1.27 2012-08-03 13:01:23 cvskris Exp $
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "sets.h"
#include "factory.h"
#include "callback.h"

namespace osg { class Node; }

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

mClass(visBase) DataManager : public CallBacker
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
    int			usePar(const IOPar&);
    			/*!<\retval 1  success
			    \retval -1 failure
			    \retval 0  warnings. */

    const char*		errMsg() const;

    void		getIds(const SoPath*,TypeSet<int>&) const;
    			/*!< Gets the ids from lowest level to highest
			     (i.e. scene ) */

    void		getIds(const std::type_info&,TypeSet<int>&) const;
    int			highestID() const;

    DataObject*		getObject(int id);
    const DataObject*	getObject(int id) const;
    DataObject*		getObject(const SoNode*);
    const DataObject*	getObject(const SoNode*) const;

    int			nrObjects() const;
    DataObject*		getIndexedObject(int idx);
    const DataObject*	getIndexedObject(int idx) const;

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
    BufferString		errmsg_;

    static const char*		sKeyFreeID();
    static const char*		sKeySelManPrefix();
};

mGlobal(visBase) DataManager& DM();

};


#endif

