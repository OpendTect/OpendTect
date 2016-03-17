#ifndef posvecdatasetfact_h
#define posvecdatasetfact_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
________________________________________________________________________

-*/

#include "posvecdatasettr.h"

defineTranslatorGroup(PosVecDataSet,"Positioned Vector Data");
defineTranslator(od,PosVecDataSet,mdTectKey);

uiString PosVecDataSetTranslatorGroup::sTypeName( int num )
{ return tr( "Positioned Vector Data", 0, num ); }

#endif
