#ifndef uisurfacesel_h
#define uisurfacesel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id: uisurfacesel.h,v 1.1 2009-04-01 11:55:32 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

#include "bufstringset.h"
#include "multiid.h"

class IOObjContext;
class uiLabeledListBox;

mClass uiSurfaceSel : public uiGroup
{
public:
			~uiSurfaceSel();

    virtual void	getSelSurfaceIds(TypeSet<MultiID>&) const;

protected:
    			uiSurfaceSel(uiParent*,const IOObjContext&);

    void		getFullList();

    uiLabeledListBox*	listfld_;
    TypeSet<MultiID>	mids_;
    BufferStringSet	names_;

    IOObjContext&	ctxt_;
};


/*! \brief ui for horizon 3D selection */

// TODO implement
mClass uiSurface3DSel : public uiSurfaceSel
{
public:
protected:
    			uiSurface3DSel(uiParent*,const IOObjContext&);
};



/*! \brief ui for horizon 2D selection */

mClass uiSurface2DSel : public uiSurfaceSel
{
public:
    void		setLineSetID(const MultiID&);
    virtual void	getSelSurfaceIds(TypeSet<MultiID>&) const;

protected:
			uiSurface2DSel(uiParent*,const IOObjContext&);

    MultiID		linesetid_;
    TypeSet<MultiID>	subselmids_;
    BufferStringSet	subselnames_;
};


mClass uiHorizon2DSel : public uiSurface2DSel
{
public:
    			uiHorizon2DSel(uiParent*);
protected:

};

#endif
