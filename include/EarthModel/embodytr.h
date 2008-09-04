#ifndef embodytr_h
#define embodytr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embodytr.h,v 1.1 2008-09-04 13:25:07 cvskris Exp $
________________________________________________________________________


-*/

#include "transl.h"
#include "emsurfaceiodata.h"

class Executor;
class IOObj;

namespace EM { class Body; }

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
		    : Translator(unm,nm)				{}
    virtual	~mcEMBodyTranslator()					{}
};



#endif
