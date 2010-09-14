#ifndef visvw2ddataman_h
#define visvw2ddataman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: visvw2ddataman.h,v 1.2 2010-09-14 09:39:22 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"

class Vw2DDataObject;


mClass Vw2DDataManager : public CallBacker
{
public:
    				Vw2DDataManager();
				~Vw2DDataManager();

    void			addObject(Vw2DDataObject*);
    void			removeObject(Vw2DDataObject*);
    void                        removeAll();

    const Vw2DDataObject*	getObject(int id) const;
    Vw2DDataObject*		getObject(int id);

    void			setSelected(Vw2DDataObject*);

    Notifier<Vw2DDataManager>	addRemove;

protected:

    void                        deSelect(int id);

    ObjectSet<Vw2DDataObject>	objects_;
    int				selectedid_;
    int				freeid_;
};

#endif
