/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2018
________________________________________________________________________

-*/



#include "uigeom2dsel.h"

#include "survgeometrytransl.h"

static IOObjContext getContext( bool forread )
{
    IOObjContext ret = mIOObjContext(SurvGeom2D);
    ret.forread_ = forread;
    return ret;
}


static uiIOObjSel::Setup getSetup( bool forread, const uiString& seltxt )
{
    uiString st = seltxt;
    if ( st.isEmpty() )
	st = forread ? od_static_tr("uiGeom2DSel","Input Geometry")
		     : od_static_tr("uiGeom2DSel","Output Geometry");
    uiIOObjSel::Setup su( st );
    return su;
}


uiGeom2DSel::uiGeom2DSel( uiParent* p, bool forread, const uiString& seltxt )
    : uiIOObjSel(p,getContext(forread),getSetup(forread,seltxt))
{
}


uiGeom2DSel::uiGeom2DSel( uiParent* p, bool forread,
			  const uiIOObjSel::Setup& su )
    : uiIOObjSel(p,getContext(forread),su)
{
}


uiGeom2DSel::~uiGeom2DSel()
{
}
