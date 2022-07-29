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

class Vw2DDataObject;
class uiFlatViewWin;
class uiFlatViewAuxDataEditor;

mExpClass(uiViewer2D) Vw2DDataManager : public CallBacker
{
public:
				Vw2DDataManager();
				~Vw2DDataManager();

    void			addObject(Vw2DDataObject*);
    void			removeObject(Vw2DDataObject*);
    void			removeAll();

    void			getObjects(ObjectSet<Vw2DDataObject>&) const;

    const Vw2DDataObject*	getObject(int id) const;
    Vw2DDataObject*		getObject(int id);

    void			setSelected(Vw2DDataObject*);
    int				selectedID()	{ return selectedid_; }

    void			usePar(const IOPar&,uiFlatViewWin*,
				    const ObjectSet<uiFlatViewAuxDataEditor>&);
    void			fillPar(IOPar&) const;

    mDefineFactory2ParamInClass(Vw2DDataObject,uiFlatViewWin*,
		    const ObjectSet<uiFlatViewAuxDataEditor>&,factory);

    Notifier<Vw2DDataManager>	addRemove;
    CNotifier<Vw2DDataManager,int> dataObjAdded;
    CNotifier<Vw2DDataManager,int> dataObjToBeRemoved;

protected:

    void			deSelect(int id);

    ObjectSet<Vw2DDataObject>	objects_;
    int				selectedid_;
    int				freeid_;

    static const char*		sKeyNrObjects()	{ return "Nr objects"; }

    bool			similarObjectPresent(
					const Vw2DDataObject*) const;
public:
    void			getObjectIDs(TypeSet<int>&) const;
};
