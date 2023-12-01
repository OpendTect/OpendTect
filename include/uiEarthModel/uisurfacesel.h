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

mExpClass(uiEarthModel) uiSurfaceSelGrp : public uiGroup
{
public:
			~uiSurfaceSelGrp();

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
			uiSurfaceSelGrp(uiParent*,const IOObjContext&);

    void		getFullList();

    uiListBox*		listfld_;
    uiListBoxFilter*	filterfld_;

    TypeSet<MultiID>	mids_;
    BufferStringSet	names_;

    IOObjContext&	ctxt_;
};


/*! \brief ui for horizon 3D selection */

// TODO implement
mExpClass(uiEarthModel) uiSurface3DSelGrp : public uiSurfaceSelGrp
{
public:
			~uiSurface3DSelGrp();

protected:
			uiSurface3DSelGrp(uiParent*,const IOObjContext&);
};



/*! \brief ui for horizon 2D selection */

mExpClass(uiEarthModel) uiSurface2DSelGrp : public uiSurfaceSelGrp
{
public:
			~uiSurface2DSelGrp();

protected:
			uiSurface2DSelGrp(uiParent*,const IOObjContext&);
};


mExpClass(uiEarthModel) uiHorizon2DSelGrp : public uiSurface2DSelGrp
{
public:
			uiHorizon2DSelGrp(uiParent*);
			~uiHorizon2DSelGrp();
protected:

};


mExpClass(uiEarthModel) uiHorizon3DSelGrp : public uiSurface3DSelGrp
{
public:
			uiHorizon3DSelGrp(uiParent*);
			~uiHorizon3DSelGrp();
protected:

};
