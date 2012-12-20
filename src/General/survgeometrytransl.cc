/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "survgeometrytransl.h"
#include "survinfo.h"
#include "survgeom.h"

using namespace Survey;

defineTranslatorGroup(SurvGeom,"Geometry");
defineTranslator(dgb2D,SurvGeom,"2D Geometry");
defineTranslator(dgb3D,SurvGeom,"3D Geometry");
mDefSimpleTranslatorSelector(SurvGeom,"Geometry");
mDefSimpleTranslatorioContext(SurvGeom,Geom);
