#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "posvecdatasettr.h"

defineTranslatorGroup(PosVecDataSet,"Positioned Vector Data");
defineTranslator(od,PosVecDataSet,mdTectKey);

uiString PosVecDataSetTranslatorGroup::sTypeName( int num )
{ return tr( "Positioned Vector Data", 0, num ); }
