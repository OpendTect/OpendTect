#ifndef embodytr_h
#define embodytr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embodytr.h,v 1.7 2009-02-02 21:52:02 cvsyuancheng Exp $
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
			mcEMBodyTranslator(const char* unm,const char* nm)
			    : Translator( unm, nm )			{}
			~mcEMBodyTranslator()				{}
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
			randposEMBodyTranslator(const char* unm,const char* nm)
    			    : Translator( unm, nm )			{}
			~randposEMBodyTranslator()			{}

    const char*		defExtension() const	{ return "rdpos"; }
    static const char*	sKeyUserName()		{ return "RandomPosBody"; }
};


#endif
