#ifndef embodytr_h
#define embodytr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embodytr.h,v 1.9 2009/07/22 16:01:14 cvsbert Exp $
________________________________________________________________________


-*/

#include "transl.h"
#include "emsurfaceiodata.h"

class Executor;
class IOObj;

namespace EM { class Body; class PolygonBody; } 

typedef EM::Body 	EMBody;

mClass EMBodyTranslatorGroup : public TranslatorGroup
{			       isTranslatorGroup(EMBody)
public:
		    	mDefEmptyTranslatorGroupConstructor(EMBody)
    const char*		defExtension() const    { return "bdy"; }
    static const char*	sKeyword()		{ return "Body"; }
};


mClass mcEMBodyTranslator : public Translator
{			    isTranslator(mc,EMBody)
public:
    			mDefEmptyTranslatorBaseConstructor( mcEMBody );
    const char*		defExtension() const	{ return "mc"; }
    static const char*  sKeyUserName()		{ return "MCBody"; }
};


mClass polygonEMBodyTranslator : public Translator
{				 isTranslator(polygon,EMBody)
public:
			polygonEMBodyTranslator(const char* unm,const char* nm);
			~polygonEMBodyTranslator();

    const char*		defExtension() const	{ return "plg"; }
    static const char*  sKeyUserName()		{ return "PolygonBody"; }
    static const IOObjContext&	getIOObjContext();

    Executor*		reader(const IOObj&,EM::PolygonBody&);
    Executor*		writer(const EM::PolygonBody&,IOObj&);

    const char*		errMsg() const;

protected:

    BufferString	errmsg_;
};


mClass randposEMBodyTranslator : public Translator
{                                isTranslator(randpos,EMBody)
public:
    			mDefEmptyTranslatorBaseConstructor( randposEMBody );
    const char*		defExtension() const	{ return "rdpos"; }
    static const char*	sKeyUserName()		{ return "RandomPosBody"; }
};


//For selection of old (3.2) mc bodies.
mClass dGBEMBodyTranslator : public Translator
{			    isTranslator(dGB,EMBody)
public:
 			mDefEmptyTranslatorBaseConstructor( dGBEMBody );
   static const char*	sKeyUserName()		{ return "dGB"; }
};



#endif
