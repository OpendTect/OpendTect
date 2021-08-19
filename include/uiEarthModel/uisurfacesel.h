#pragma once

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

    void		getChosen(TypeSet<MultiID>&) const;
    void		setChosen(const TypeSet<MultiID>&);
    int			nrChosen() const;

    mDeprecated("Use getChosen")
    virtual void	getSelSurfaceIds(TypeSet<MultiID>&) const;
    mDeprecated("Use setChosen")
    void		setSelSurfaceIds(const TypeSet<MultiID>&);
    mDeprecated("Use nrChosen")
    int 		getSelItems() const;

    void		removeFromList(const TypeSet<MultiID>&);
    void		clearList();

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


