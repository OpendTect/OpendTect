#ifndef embodytr_h
#define embodytr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embodytr.h,v 1.3 2008-09-09 17:22:02 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "transl.h"
#include "emsurfaceiodata.h"

class Executor;
class IOObj;

namespace EM { class Body; class PolygonBody; }

typedef EM::Body 	EMBody;

class EMBodyTranslatorGroup : public TranslatorGroup
{		    isTranslatorGroup(EMBody)
public:
		    mDefEmptyTranslatorGroupConstructor(EMBody)

    const char*		defExtension() const	{ return "mc"; }
    static const char*	sKeyword();
};


class mcEMBodyTranslator : public Translator
{				 isTranslator(mc,EMBody)
public:
			mcEMBodyTranslator(const char* unm,const char* nm)
			    : Translator(unm,nm)			{}
			~mcEMBodyTranslator()				{}
    static const char*  sKeyUserName() { return "MCBody"; }
};


class polygonEMBodyTranslator : public Translator
{                               isTranslator(polygon,EMBody)
public:
			polygonEMBodyTranslator(const char* unm,const char* nm);
			~polygonEMBodyTranslator();

    static const char*  sKeyUserName();
    static const IOObjContext&	getIOObjContext();

    Executor*		reader(const IOObj&,EM::PolygonBody&);
    Executor*		writer(const EM::PolygonBody&,IOObj&);

    const char*		errMsg() const;

protected:

    BufferString	errmsg_;
};


#endif
