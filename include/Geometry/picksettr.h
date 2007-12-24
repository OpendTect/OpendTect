#ifndef picksettr_h
#define picksettr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: picksettr.h,v 1.8 2007-12-24 16:49:58 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include "bufstringset.h"
class Conn;
namespace Pick { class Set; }
class BinIDValueSet;
template <class T> class ODPolygon;


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
    static ODPolygon<float>* getPolygon(const IOObj&,BufferString& errmsg);
    			//!< Returns null on failure
};


class dgbPickSetTranslator : public PickSetTranslator
{			     isTranslator(dgb,PickSet)
public:

    			mDefEmptyTranslatorConstructor(dgb,PickSet)

    const char*		read(Pick::Set&,Conn&);
    const char*		write(const Pick::Set&,Conn&);

};


#endif
