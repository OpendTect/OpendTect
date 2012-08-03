#ifndef uihorgeom2attr_h
#define uihorgeom2attr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2011
 RCS:           $Id: uihorgeom2attr.h,v 1.2 2012-08-03 13:00:56 cvskris Exp $
________________________________________________________________________

-*/


#include "uiearthmodelmod.h"
#include "uiselsimple.h"
class uiGenInput;
class uiListBox;
class DataPointSet;
namespace EM { class Horizon3D; }


/*!\brief Save the geometry to an attribute */

mClass(uiEarthModel) uiHorGeom2Attr : public uiGetObjectName
{
public:
			uiHorGeom2Attr(uiParent*,EM::Horizon3D&);
			~uiHorGeom2Attr();

protected:

    EM::Horizon3D&	hor_;
    BufferStringSet*	itmnms_;

    uiGenInput*		msfld_;

    BufferStringSet&	getItems(const EM::Horizon3D&);

    virtual bool	acceptOK(CallBacker*);

};


/*!\brief Change the geometry using an attribute */

mClass(uiEarthModel) uiHorAttr2Geom : public uiDialog
{
public:
			uiHorAttr2Geom(uiParent*,EM::Horizon3D&,
					const DataPointSet&,int colid);

protected:

    EM::Horizon3D&	hor_;
    const DataPointSet&	dps_;
    int			colid_;

    uiGenInput*		isdeltafld_;
    uiGenInput*		msfld_;

    virtual bool	acceptOK(CallBacker*);


};


#endif

