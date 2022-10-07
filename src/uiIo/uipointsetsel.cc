/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipointsetsel.h"

#include "picksettr.h"


static IOObjContext getContext( bool forread, bool ispoly )
{
    IOObjContext ret = mIOObjContext(PickSet);
    PickSetTranslator::fillConstraints( ret, ispoly );
    ret.forread_ = forread;
    return ret;
}


static uiIOObjSel::Setup getSetup( bool forread, const uiString& seltxt,
				   bool ispoly )
{
    uiString st = seltxt;
    if ( st.isEmpty() )
    {
	if ( ispoly )
	    st = forread ? od_static_tr("uiPolygonSel","Input Polygon")
			 : od_static_tr("uiPolygonSel","Output Polygon");
	else
	    st = forread ? od_static_tr("uiPointSetSel","Input PointSet")
			 : od_static_tr("uiPointSetSel","Output PointSet");
    }
    uiIOObjSel::Setup su( st );
    return su;
}



// uiPointSetPolygonSel
uiPointSetPolygonSel::uiPointSetPolygonSel( uiParent* p, bool forread,
					    bool ispoly, const uiString& seltxt)
    : uiIOObjSel(p,getContext(forread,ispoly),getSetup(forread,seltxt,ispoly))
{}


uiPointSetPolygonSel::uiPointSetPolygonSel( uiParent* p, bool forread,
				bool ispoly, const uiIOObjSel::Setup& su )
    : uiIOObjSel(p,getContext(forread,ispoly),su)
{}


uiPointSetPolygonSel* uiPointSetPolygonSel::create( uiParent* p, bool forread,
					bool ispoly, const uiString& seltxt )
{
    if ( ispoly )
	return new uiPolygonSel( p, forread, seltxt );

    return new uiPointSetSel( p, forread, seltxt );
}


uiPointSetPolygonSel* uiPointSetPolygonSel::create( uiParent* p, bool forread,
				bool ispoly, const uiIOObjSel::Setup& su )
{
    if ( ispoly )
	return new uiPolygonSel( p, forread, su );

    return new uiPointSetSel( p, forread, su );
}


uiPointSetPolygonSel::~uiPointSetPolygonSel()
{}



// uiPointSetSel
uiPointSetSel::uiPointSetSel( uiParent* p, bool forread,
			      const uiString& seltxt )
    : uiPointSetPolygonSel(p,forread,false,seltxt)
{
}


uiPointSetSel::uiPointSetSel( uiParent* p, bool forread,
			      const uiIOObjSel::Setup& su )
    : uiPointSetPolygonSel(p,forread,false,su)
{
}


uiPointSetSel::~uiPointSetSel()
{
}



// uiPolygonSel
uiPolygonSel::uiPolygonSel( uiParent* p, bool forread,
			    const uiString& seltxt )
    : uiPointSetPolygonSel(p,forread,true,seltxt)
{
}


uiPolygonSel::uiPolygonSel( uiParent* p, bool forread,
			    const uiIOObjSel::Setup& su )
    : uiPointSetPolygonSel(p,forread,true,su)
{
}


uiPolygonSel::~uiPolygonSel()
{
}
