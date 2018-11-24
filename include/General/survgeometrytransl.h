#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal / Bert
 Date:		Dec 2012 / Nov 2018
________________________________________________________________________


-*/

#include "generalmod.h"
#include "survgeom2d.h"
#include "transl.h"


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

    mUseType( Survey,	Geometry2D );
    mUseType( Survey,	GeomID );

			mDefEmptyTranslatorBaseConstructor(SurvGeom2D);

    virtual Geometry2D*	readGeometry(const IOObj&,uiString&) const = 0;
    virtual bool	writeGeometry(IOObj&,const Geometry2D&,
				      uiString&) const		   = 0;

    static GeomID	getGeomID(const IOObj&);
    static IOObj*	getIOObj(GeomID);
    static IOObj*	getEntry(const char* objname,const char* trnm);

};


mExpClass(General) dgbSurvGeom2DTranslator : public SurvGeom2DTranslator
{
			isTranslator(dgb,SurvGeom2D);
public:
			dgbSurvGeom2DTranslator(const char* s1,const char* s2)
			    : SurvGeom2DTranslator(s1,s2)	{}

    const char*		defExtension() const	{ return "geom"; }
    Geometry2D*		readGeometry(const IOObj&,uiString&) const;
    bool		writeGeometry(IOObj&,const Geometry2D&,
					uiString&) const;

};
