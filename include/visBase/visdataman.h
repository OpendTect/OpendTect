#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "callback.h"
#include "factory.h"
#include "integerid.h"
#include "sets.h"
#include <typeinfo>
#include <unordered_map>

namespace osg { class Node; }

namespace visBase
{
class DataObject;
class SelectionManager;

/*!\brief the visBase Data Manager */

mExpClass(visBase) DataManager : public CallBacker
{
public:
			DataManager();
			~DataManager();

    const char*		errMsg() const;

    void		getIDs(const std::type_info&,TypeSet<VisID>&) const;
    VisID		highestID() const;

    DataObject*		getObject(const VisID&);
    const DataObject*	getObject(const VisID&) const;
    VisID		getID(const osg::Node*) const;
			//!<Returns VisID::udf() if not found

    int			nrObjects() const;
    DataObject*		getIndexedObject(int idx);
    const DataObject*	getIndexedObject(int idx) const;

    SelectionManager&	selMan() { return selman_; }

    void		fillPar(IOPar&) const;
			//Only saves freeid_
    bool		usePar(const IOPar&);
			//Only restores freeid_

    Notifier<DataManager> removeallnotify;

    mDefineFactoryInClass( DataObject, factory );

private:

    friend class	DataObject;
    void		addObject(DataObject*);
    void		removeObject(DataObject*);

    mutable WeakPtrSet<DataObject> objects_;

    std::unordered_map<DataObject*,int> objidxmap_;

    int				freeid_ = 0;
    SelectionManager&		selman_;
    BufferString		errmsg_;

    static const char*		sKeyFreeID();
    static const char*		sKeySelManPrefix();
};

mGlobal(visBase) DataManager& DM();

} // namespace visBase
