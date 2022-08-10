#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "callback.h"
#include "factory.h"
#include "emposid.h"

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
    void			getObjectIDs(TypeSet<int>&) const;

    const DataObject*		getObject(int id) const;
    DataObject*			getObject(int id);

    void			setSelected(DataObject*);
    int				selectedID()	{ return selectedid_; }

    void			usePar(const IOPar&,uiFlatViewWin*,
				    const ObjectSet<uiFlatViewAuxDataEditor>&);
    void			fillPar(IOPar&) const;

    mDefineFactory2ParamInClass(DataObject,uiFlatViewWin*,
		    const ObjectSet<uiFlatViewAuxDataEditor>&,factory);

    Notifier<DataManager>	addRemove;
    CNotifier<DataManager,int> dataObjAdded;
    CNotifier<DataManager,int> dataObjToBeRemoved;

protected:

    void			deSelect(int id);

    ObjectSet<DataObject>	objects_;
    int				selectedid_;
    int				freeid_;

    static const char*		sKeyNrObjects()	{ return "Nr objects"; }

    bool			similarObjectPresent(const DataObject*) const;
};

} // namespace View2D
