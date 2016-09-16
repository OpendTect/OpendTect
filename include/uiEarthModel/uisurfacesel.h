#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uigroup.h"

#include "bufstringset.h"
#include "dbkey.h"

class IOObjContext;
class uiListBox;

mExpClass(uiEarthModel) uiSurfaceSel : public uiGroup
{
public:
			~uiSurfaceSel();

    virtual void	getSelSurfaceIds(DBKeySet&) const;
    void		setSelSurfaceIds(const DBKeySet&);
    int 		getSelItems() const;

    void		removeFromList(const DBKeySet&);
    void		clearList();

protected:
    			uiSurfaceSel(uiParent*,const IOObjContext&);

    void		getFullList();

    uiListBox*		listfld_;
    DBKeySet	mids_;
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
protected:
			uiSurface2DSel(uiParent*,const IOObjContext&);
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
