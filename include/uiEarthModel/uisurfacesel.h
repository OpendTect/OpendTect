#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uigroup.h"

#include "bufstringset.h"
#include "multiid.h"

class IOObjContext;
class uiListBox;
class uiListBoxFilter;

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
    uiListBoxFilter*	filterfld_;

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
