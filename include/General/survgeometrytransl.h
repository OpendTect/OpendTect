#ifndef survgeometrytransl_h
#define survgeometrytransl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Dec 2012
 RCS:		$Id$
________________________________________________________________________


-*/

#include "generalmod.h"
#include "survgeom.h"
#include "transl.h"

namespace Survey
{

mExpClass(General) SurvGeomTranslatorGroup : public TranslatorGroup
{
			isTranslatorGroup(SurvGeom);

public:
    			mDefEmptyTranslatorGroupConstructor(SurvGeom);
    const char*		defExtension() const	{ return "geom"; }
    static FixedString	keyword();
};


mExpClass(General) SurvGeomTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(SurvGeom);
};


mExpClass(General) dgb2DSurvGeomTranslator : public SurvGeomTranslator
{
    			isTranslator(dgb2D,SurvGeom);
public:
			mDefEmptyTranslatorConstructor(dgb2D,SurvGeom);

};


mExpClass(General) dgb3DSurvGeomTranslator : public SurvGeomTranslator
{
    			isTranslator(dgb3D,SurvGeom);
public:
			mDefEmptyTranslatorConstructor(dgb3D,SurvGeom);

};

}

#endif
