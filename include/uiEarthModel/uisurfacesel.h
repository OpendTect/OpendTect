#ifndef uisurfacesel_h
#define uisurfacesel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uigroup.h"

#include "bufstringset.h"
#include "multiid.h"

class IOObjContext;
class uiListBox;

mExpClass(uiEarthModel) uiSurfaceSel : public uiGroup
{
public:
			~uiSurfaceSel();

    virtual void	getSelSurfaceIds(TypeSet<MultiID>&) const;
    int 		getSelItems() const;

    void		removeFromList(const TypeSet<MultiID>&);

protected:
    			uiSurfaceSel(uiParent*,const IOObjContext&);

    void		getFullList();

    uiListBox*		listfld_;
    TypeSet<MultiID>	mids_;
    BufferStringSet	names_;

    IOObjContext&	ctxt_;
};


/*! \brief ui for horizon 3D selection */

// TODO implement
mExpClass(uiEarthModel) uiSurface3DSel : public uiSurfaceSel
{
public:
protected:
    			uiSurface3DSel(uiParent*,const IOObjContext&);
};



/*! \brief ui for horizon 2D selection */

mExpClass(uiEarthModel) uiSurface2DSel : public uiSurfaceSel
{
public:
    void		setLineSetID(const MultiID&);

protected:
			uiSurface2DSel(uiParent*,const IOObjContext&);

    MultiID		linesetid_;
    TypeSet<MultiID>	subselmids_;
    BufferStringSet	subselnames_;
};


mExpClass(uiEarthModel) uiHorizon2DSel : public uiSurface2DSel
{
public:
    			uiHorizon2DSel(uiParent*);
protected:

};


mExpClass(uiEarthModel) uiHorizon3DSel : public uiSurface3DSel
{
public:
    			uiHorizon3DSel(uiParent*);
protected:

};


#endif

