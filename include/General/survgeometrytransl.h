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

//TODO: Make Translator/Group classes for 3D when needed.

mExpClass(General) SurvGeom2DTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(SurvGeom2D);
    mODTextTranslationClass(SurvGeom2DTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(SurvGeom2D);
    const char*		defExtension() const	{ return "geom"; }
};


mExpClass(General) SurvGeom2DTranslator : public Translator
{
public:
				mDefEmptyTranslatorBaseConstructor(SurvGeom2D);

    virtual Survey::Geometry*	readGeometry(const IOObj&,uiString&) const = 0;
    virtual bool		writeGeometry(IOObj&,Survey::Geometry&,
					      uiString&) const		   = 0;

    static Pos::GeomID	getGeomID(const IOObj&);
    static IOObj*	getIOObj(Pos::GeomID);
    static IOObj*	createEntry(const char* objname,const char* trnm);

};


mExpClass(General) dgbSurvGeom2DTranslator : public SurvGeom2DTranslator
{
			isTranslator(dgb,SurvGeom2D);
public:
			dgbSurvGeom2DTranslator(const char* s1,const char* s2)
			    : SurvGeom2DTranslator(s1,s2)	{}

    const char*		defExtension() const	{ return "geom"; }
    Survey::Geometry*	readGeometry(const IOObj&,uiString&) const;
    bool		writeGeometry(IOObj&,Survey::Geometry&,uiString&) const;

};


#endif

