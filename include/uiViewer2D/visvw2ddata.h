
#ifndef visvw2ddata_h
#define visvw2ddata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: visvw2ddata.h,v 1.1 2010-06-24 08:37:17 cvsumesh Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "refcount.h"


mClass Vw2DDataObject : public CallBacker
{ mRefCountImpl(Vw2DDataObject)
public:
    				Vw2DDataObject();

    int				id() const		{ return id_; }
    void			setID(int nid)		{ id_ = nid; }

    const char*			name() const;
    virtual void		setName(const char*);

    virtual NotifierAccess*	deSelection()		{ return 0; }

protected:

    virtual void	triggerDeSel()			{}

    int			id_;
    BufferString*	name_;

    friend class	Vw2DDataManager;
};


#endif
