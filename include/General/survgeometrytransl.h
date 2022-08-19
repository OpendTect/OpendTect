#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    virtual bool	isUsable() const { return true; }

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
    bool		implRename(const IOObj*,const char*) const override;
};
