#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
