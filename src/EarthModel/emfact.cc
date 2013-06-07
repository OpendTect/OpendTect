/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id$";


#include "emsurfacetr.h"
defineTranslatorGroup(EMHorizon2D,EMHorizon2DTranslatorGroup::keyword());
defineTranslatorGroup(EMHorizon3D,EMHorizon3DTranslatorGroup::keyword());
defineTranslatorGroup(EMAnyHorizon,EMAnyHorizonTranslatorGroup::keyword());
defineTranslatorGroup(EMFaultStickSet,EMFaultStickSetTranslatorGroup::keyword());
defineTranslatorGroup(EMFault3D,EMFault3DTranslatorGroup::keyword());

defineTranslator(dgb,EMHorizon2D,mDGBKey);
defineTranslator(dgb,EMHorizon3D,mDGBKey);
defineTranslator(dgb,EMFaultStickSet,mDGBKey);
defineTranslator(dgb,EMFault3D,mDGBKey);

#include "lmkemfaulttransl.h"
defineTranslator(lmk,EMFault3D,"Landmark");

#include "embodytr.h"
defineTranslatorGroup(EMBody,EMBodyTranslatorGroup::sKeyword());
defineTranslator(mc,EMBody,mcEMBodyTranslator::sKeyUserName());
defineTranslator(randpos,EMBody,randposEMBodyTranslator::sKeyUserName());
defineTranslator(polygon,EMBody,polygonEMBodyTranslator::sKeyUserName());
defineTranslator(dGB,EMBody,dGBEMBodyTranslator::sKeyUserName() );
