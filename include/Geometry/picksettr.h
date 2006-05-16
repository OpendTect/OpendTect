#ifndef picksettr_h
#define picksettr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: picksettr.h,v 1.7 2006-05-16 16:28:22 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include "bufstringset.h"
class Conn;
namespace Pick { class Set; }
class BinIDValueSet;


class PickSetTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(PickSet)
public:
    			mDefEmptyTranslatorGroupConstructor(PickSet)

    const char*		defExtension() const		{ return "pck"; }
};


class PickSetTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(PickSet)

    virtual const char*	read(Pick::Set&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const Pick::Set&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Pick::Set&,const IOObj*,BufferString&);
    static bool		store(const Pick::Set&,const IOObj*,BufferString&);

    static void		createBinIDValueSets(const BufferStringSet& ioobjids,
					     ObjectSet<BinIDValueSet>&);
    			//!< Utility function.
};


class dgbPickSetTranslator : public PickSetTranslator
{			     isTranslator(dgb,PickSet)
public:

    			mDefEmptyTranslatorConstructor(dgb,PickSet)

    const char*		read(Pick::Set&,Conn&);
    const char*		write(const Pick::Set&,Conn&);

};


#endif
