#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Dec 2012
________________________________________________________________________


-*/

#include "generalmod.h"
#include "survgeom.h"
#include "transl.h"

//TODO: Make Translator/Group classes for 3D when needed.

mExpClass(General) SurvGeom2DTranslatorGroup : public TranslatorGroup
{
			isTranslatorGroup(SurvGeom2D);
public:
			mDefEmptyTranslatorGroupConstructor(SurvGeom2D);
    const char*		defExtension() const override	{ return "geom"; }
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

    const char*		defExtension() const override	{ return "geom"; }
    Survey::Geometry*	readGeometry(const IOObj&,uiString&) const override;
    bool		writeGeometry(IOObj&,Survey::Geometry&,
				      uiString&) const override;
    bool		implRename(const IOObj*,const char*,
				   const CallBack* cb=0) const override;
};


