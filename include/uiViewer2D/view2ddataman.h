#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"

#include "factory.h"
#include "emposid.h"
#include "view2ddata.h"

class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

namespace View2D
{

class DataObject;

mExpClass(uiViewer2D) DataManager : public CallBacker
{
public:
				DataManager();
				~DataManager();

    void			addObject(DataObject*);
    void			removeObject(DataObject*);
    void			removeAll();

    void			getObjects(ObjectSet<DataObject>&) const;
    void			getObjectIDs(TypeSet<Vis2DID>&) const;

    const DataObject*		getObject(const Vis2DID&) const;
    DataObject*			getObject(const Vis2DID&);

    void			setSelected(DataObject*);
    Vis2DID			selectedID()	{ return selectedid_; }

    void			usePar(const IOPar&,uiFlatViewWin*,
				    const ObjectSet<uiFlatViewAuxDataEditor>&);
    void			fillPar(IOPar&) const;

    mDefineFactory2ParamInClass(DataObject,uiFlatViewWin*,
		    const ObjectSet<uiFlatViewAuxDataEditor>&,factory);

    Notifier<DataManager>	addRemove;
    CNotifier<DataManager,Vis2DID> dataObjAdded;
    CNotifier<DataManager,Vis2DID> dataObjToBeRemoved;

protected:

    void			deSelect(const Vis2DID&);

    RefObjectSet<DataObject>	objects_;
    Vis2DID			selectedid_;
    int				freeid_		= 0;

    static const char*		sKeyNrObjects()	{ return "Nr objects"; }

    bool			similarObjectPresent(const DataObject*) const;
};

} // namespace View2D
