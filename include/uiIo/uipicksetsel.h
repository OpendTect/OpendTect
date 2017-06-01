#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiioobjsel.h"
#include "pickset.h"
#include "polygon.h"
#include "ioobjctxt.h"


/*!\brief selects pick set or polygon. */

mExpClass(uiIo) uiPickSetIOObjSel : public uiIOObjSel
{ mODTextTranslationClass(uiPickSetIOObjSel);
public:

    enum Type		{ PolygonOnly, NoPolygon, AllSets };

			uiPickSetIOObjSel(uiParent*,bool forread=true,
					  Type t=AllSets,const char* cat=0);
			uiPickSetIOObjSel(uiParent*,const Setup&,
					  bool forread=true,Type t=AllSets,
					  const char* cat=0);

    ConstRefMan<Pick::Set> getPickSet(bool emptyisok=false) const;
    RefMan<Pick::Set>	getPickSetForEdit(bool emptyisok=false) const;
    ODPolygon<float>*	getSelectionPolygon() const;
    ODPolygon<double>*	getCoordPolygon() const;

    static IOObjContext	getCtxt(Type t=NoPolygon,bool forread=true,
				const char* cat=0,const char* transl=0);
    static void		updateCtxt(IOObjContext&,Type t=NoPolygon,
				   bool forread=true,const char* cat=0,
				   const char* transl=0);

};
