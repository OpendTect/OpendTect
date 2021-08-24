/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/



#include "emsurfacetr.h"
defineTranslatorGroup(EMHorizon2D,"2D Horizon");
defineTranslatorGroup(EMHorizon3D,"Horizon");
defineTranslatorGroup(EMAnyHorizon,"Any Horizon");
defineTranslatorGroup(EMFaultStickSet,"FaultStickSet");
defineTranslatorGroup(EMFault3D,"Fault");
defineTranslatorGroup(EMFaultSet3D,"FaultSet");

defineTranslator(dgb,EMHorizon2D,mDGBKey);
defineTranslator(dgb,EMHorizon3D,mDGBKey);
defineTranslator(dgb,EMFaultStickSet,mDGBKey);
defineTranslator(dgb,EMFault3D,mDGBKey);
defineTranslator(dgb,EMFaultSet3D,mDGBKey);

#include "lmkemfaulttransl.h"
defineTranslator(lmk,EMFault3D,"Landmark");

#include "embodytr.h"
#include "emmarchingcubessurface.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
defineTranslatorGroup(EMBody,"Body");
defineTranslator(od,EMBody,EMBodyTranslatorGroup::sKeyUserWord());
defineTranslator(mc,EMBody,EMBodyTranslatorGroup::sKeyUserWord());
defineTranslator(randpos,EMBody,EMBodyTranslatorGroup::sKeyUserWord());
defineTranslator(polygon,EMBody,EMBodyTranslatorGroup::sKeyUserWord());
