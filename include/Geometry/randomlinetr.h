#ifndef randomlinetr_h
#define randomlinetr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: randomlinetr.h,v 1.2 2007-11-05 15:20:05 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include "bufstringset.h"

namespace Geometry { class RandomLineSet; }
class Conn;

class RandomLineSetTranslatorGroup : public TranslatorGroup
{				  isTranslatorGroup(RandomLineSet)
public:
    			mDefEmptyTranslatorGroupConstructor(RandomLineSet)

    const char*		defExtension() const		{ return "rdl"; }
};


class RandomLineSetTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(RandomLineSet)

    virtual const char*	read(Geometry::RandomLineSet&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const Geometry::RandomLineSet&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Geometry::RandomLineSet&,const IOObj*,
	    			 BufferString&);
    static bool		store(const Geometry::RandomLineSet&,const IOObj*,
	    		      BufferString&);
};


class dgbRandomLineSetTranslator : public RandomLineSetTranslator
{				isTranslator(dgb,RandomLineSet)
public:

    			mDefEmptyTranslatorConstructor(dgb,RandomLineSet)

    const char*		read(Geometry::RandomLineSet&,Conn&);
    const char*		write(const Geometry::RandomLineSet&,Conn&);
};


#endif
