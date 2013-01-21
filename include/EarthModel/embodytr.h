#ifndef embodytr_h
#define embodytr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "transl.h"
#include "emsurfaceiodata.h"

class Executor;
class IOObj;

namespace EM { class Body; class PolygonBody; } 

typedef EM::Body 	EMBody;

/*!
\brief TranslatorGroup for EM::Body.
*/

mExpClass(EarthModel) EMBodyTranslatorGroup : public TranslatorGroup
{			       isTranslatorGroup(EMBody)
public:
		    	mDefEmptyTranslatorGroupConstructor(EMBody)
    const char*		defExtension() const    { return "bdy"; }
    static const char*	sKeyword()		{ return "Body"; }
};


/*!
\brief Marching cubes EM::Body Translator.
*/

mExpClass(EarthModel) mcEMBodyTranslator : public Translator
{			    isTranslator(mc,EMBody)
public:
    			mDefEmptyTranslatorBaseConstructor( mcEMBody );
    const char*		defExtension() const	{ return "mc"; }
    static FixedString  sKeyUserName()		{ return "MCBody"; }
};


/*!
\brief EM::PolygonBody Translator.
*/

mExpClass(EarthModel) polygonEMBodyTranslator : public Translator
{				 isTranslator(polygon,EMBody)
public:
			polygonEMBodyTranslator(const char* unm,const char* nm);
			~polygonEMBodyTranslator();

    const char*		defExtension() const	{ return "plg"; }
    static FixedString  sKeyUserName()		{ return "PolygonBody"; }
    static const IOObjContext&	getIOObjContext();

    Executor*		reader(const IOObj&,EM::PolygonBody&);
    Executor*		writer(const EM::PolygonBody&,IOObj&);

    const char*		errMsg() const;

protected:

    BufferString	errmsg_;
};


/*!
\brief Random position EM::Body Translator.
*/

mExpClass(EarthModel) randposEMBodyTranslator : public Translator
{                                isTranslator(randpos,EMBody)
public:
    			mDefEmptyTranslatorBaseConstructor( randposEMBody );
    const char*		defExtension() const	{ return "rdpos"; }
    static FixedString	sKeyUserName()		{ return "RandomPosBody"; }
};


/*!
\brief For selection of old (3.2) marchingcube (mc) bodies.
*/

mExpClass(EarthModel) dGBEMBodyTranslator : public Translator
{			    isTranslator(dGB,EMBody)
public:
 			mDefEmptyTranslatorBaseConstructor( dGBEMBody );
   static const char*	sKeyUserName()		{ return "dGB"; }
};


#endif

