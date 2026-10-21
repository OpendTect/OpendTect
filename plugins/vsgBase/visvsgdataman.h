#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgselman.h"

#include "callback.h"
#include "factory.h"
#include "integerid.h"
#include "objectset.h"
#include "typeset.h"

#include <typeinfo>
#include <unordered_map>

namespace vsg { class Node; }

namespace dGBVis
{

class DataObject;

/*!\brief Keeps track of all dGBVis objects by id. Separate from
  visBase::DM(), so a dGBVis id never refers to a visBase object. */

mExpClass(vsgBase) DataManager : public CallBacker
{
public:
			DataManager();
			~DataManager();

    void		getIDs(const std::type_info&,TypeSet<VisID>&) const;
    VisID		highestID() const;
    void		updateID(const VisID& oldid,const VisID& newid);

    DataObject*		getObject(const VisID&);
    const DataObject*	getObject(const VisID&) const;
    VisID		getID(const vsg::Node*) const;
			//!<Returns VisID::udf() if not found

    SelectionManager&	selMan()			{ return selman_; }
    const SelectionManager& selMan() const		{ return selman_; }

    int			nrObjects() const;
    DataObject*		getIndexedObject(int idx);
    const DataObject*	getIndexedObject(int idx) const;

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

    ObjectSet<DataObject>		objects_;
    std::unordered_map<int,DataObject*> visidobjmap_;
    int					freeid_ = 0;
    SelectionManager&			selman_;

    static const char*	sKeyFreeID();
};

mGlobal(vsgBase) DataManager& DM();

mGlobal(vsgBase) void initStdClasses();

} // namespace dGBVis
