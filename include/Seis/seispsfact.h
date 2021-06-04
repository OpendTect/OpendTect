#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2005
________________________________________________________________________

-*/

#include "seispsioprov.h"
#include "seismulticubeps.h"
#include "segydirecttr.h"

#include "uistrings.h"

defineTranslatorGroup(SeisPS3D,"Pre-Stack Seismics");
mDefSimpleTranslatorSelector(SeisPS3D);
uiString SeisPS3DTranslatorGroup::sTypeName( int num)
{ return uiStrings::sVolDataName(false,true,true,false,false); }

defineTranslator(CBVS,SeisPS3D,"CBVS");
defineTranslator(MultiCube,SeisPS3D,"MultiCube");
defineTranslator(SEGYDirect,SeisPS3D,mSEGYDirectTranslNm);

defineTranslatorGroup(SeisPS2D,"2D Pre-Stack Seismics");
mDefSimpleTranslatorSelector(SeisPS2D);
uiString SeisPS2DTranslatorGroup::sTypeName( int num )
{ return uiStrings::sVolDataName(true,false,true,false,false); }

defineTranslator(CBVS,SeisPS2D,"CBVS");
defineTranslator(SEGYDirect,SeisPS2D,mSEGYDirectTranslNm);


