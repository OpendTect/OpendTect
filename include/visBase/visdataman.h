#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "visbasecommon.h"
#include "factory.h"
#include "notify.h"
#include <typeinfo>
namespace osg { class Node; }


namespace visBase
{
class SelectionManager;

/*!\brief the visBase Data Manager */

mExpClass(visBase) DataManager : public CallBacker
{
public:
			DataManager();
    virtual		~DataManager();

    const char*		errMsg() const;

    void		getIDs(const std::type_info&,TypeSet<int>&) const;
    int			highestID() const;

    DataObject*		getObject(int id);
    const DataObject*	getObject(int id) const;
    int			getID(const osg::Node*) const;
			//!<Returns -1 if not found

    int			nrObjects() const;
    DataObject*		getIndexedObject(int idx);
    const DataObject*	getIndexedObject(int idx) const;

    SelectionManager&	selMan() { return selman_; }

    void		fillPar(IOPar&) const;
			//Only saves freeid_
    bool		usePar(const IOPar&);
			//Only restores freeid_

    Notifier<DataManager>	removeallnotify;

    mDefineFactoryInClass( DataObject, factory );
protected:

    friend class	DataObject;
    void		addObject( DataObject* );
    void		removeObject( DataObject* );

    ObjectSet<DataObject>	objects_;
    mutable int			prevobjectidx_;

    int				freeid_;
    SelectionManager&		selman_;
    BufferString		errmsg_;

    static const char*		sKeyFreeID();
    static const char*		sKeySelManPrefix();
};

mGlobal(visBase) DataManager& DM();

};
