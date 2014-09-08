/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "emsurfacetr.h"
defineTranslatorGroup(EMHorizon2D,EMHorizon2DTranslatorGroup::keyword());
defineTranslatorGroup(EMHorizon3D,EMHorizon3DTranslatorGroup::keyword());
defineTranslatorGroup(EMAnyHorizon,EMAnyHorizonTranslatorGroup::keyword());
defineTranslatorGroup(EMFaultStickSet,
	EMFaultStickSetTranslatorGroup::keyword());
defineTranslatorGroup(EMFault3D,EMFault3DTranslatorGroup::keyword());

defineTranslator(dgb,EMHorizon2D,mDGBKey);
defineTranslator(dgb,EMHorizon3D,mDGBKey);
defineTranslator(dgb,EMFaultStickSet,mDGBKey);
defineTranslator(dgb,EMFault3D,mDGBKey);

#include "lmkemfaulttransl.h"
defineTranslator(lmk,EMFault3D,"Landmark");

#include "embodytr.h"
#include "emmarchingcubessurface.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
defineTranslatorGroup(EMBody,EMBodyTranslatorGroup::keyword());
defineTranslator(od,EMBody,EMBodyTranslatorGroup::sKeyUserWord());
defineTranslator(mc,EMBody,EMBodyTranslatorGroup::sKeyUserWord());
defineTranslator(randpos,EMBody,EMBodyTranslatorGroup::sKeyUserWord());
defineTranslator(polygon,EMBody,EMBodyTranslatorGroup::sKeyUserWord());
