#ifndef randomlinetr_h
#define randomlinetr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: randomlinetr.h,v 1.4 2009/07/22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include "bufstringset.h"

namespace Geometry { class RandomLineSet; }
class Conn;

mClass RandomLineSetTranslatorGroup : public TranslatorGroup
{				  isTranslatorGroup(RandomLineSet)
public:
    			mDefEmptyTranslatorGroupConstructor(RandomLineSet)

    const char*		defExtension() const		{ return "rdl"; }
};


mClass RandomLineSetTranslator : public Translator
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


mClass dgbRandomLineSetTranslator : public RandomLineSetTranslator
{				isTranslator(dgb,RandomLineSet)
public:

    			mDefEmptyTranslatorConstructor(dgb,RandomLineSet)

    const char*		read(Geometry::RandomLineSet&,Conn&);
    const char*		write(const Geometry::RandomLineSet&,Conn&);
};


#endif
