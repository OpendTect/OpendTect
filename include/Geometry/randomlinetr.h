#ifndef randomlinetr_h
#define randomlinetr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: randomlinetr.h,v 1.1 2006-12-14 21:44:35 cvsnanne Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include "bufstringset.h"

namespace Geometry { class RandomLine; }
class Conn;

class RandomLineTranslatorGroup : public TranslatorGroup
{				  isTranslatorGroup(RandomLine)
public:
    			mDefEmptyTranslatorGroupConstructor(RandomLine)

    const char*		defExtension() const		{ return "rdl"; }
};


class RandomLineTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(RandomLine)

    virtual const char*	read(Geometry::RandomLine&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const Geometry::RandomLine&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Geometry::RandomLine&,const IOObj*,
	    			 BufferString&);
    static bool		store(const Geometry::RandomLine&,const IOObj*,
	    		      BufferString&);
};


class dgbRandomLineTranslator : public RandomLineTranslator
{				isTranslator(dgb,RandomLine)
public:

    			mDefEmptyTranslatorConstructor(dgb,RandomLine)

    const char*		read(Geometry::RandomLine&,Conn&);
    const char*		write(const Geometry::RandomLine&,Conn&);
};


#endif
