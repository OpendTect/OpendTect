#ifndef randomlinetr_h
#define randomlinetr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: randomlinetr.h,v 1.5 2012-08-03 13:00:28 cvskris Exp $
________________________________________________________________________

-*/
 
#include "geometrymod.h"
#include "transl.h"
#include "bufstringset.h"

namespace Geometry { class RandomLineSet; }
class Conn;

mClass(Geometry) RandomLineSetTranslatorGroup : public TranslatorGroup
{				  isTranslatorGroup(RandomLineSet)
public:
    			mDefEmptyTranslatorGroupConstructor(RandomLineSet)

    const char*		defExtension() const		{ return "rdl"; }
};


mClass(Geometry) RandomLineSetTranslator : public Translator
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


mClass(Geometry) dgbRandomLineSetTranslator : public RandomLineSetTranslator
{				isTranslator(dgb,RandomLineSet)
public:

    			mDefEmptyTranslatorConstructor(dgb,RandomLineSet)

    const char*		read(Geometry::RandomLineSet&,Conn&);
    const char*		write(const Geometry::RandomLineSet&,Conn&);
};


#endif

