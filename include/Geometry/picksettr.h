#ifndef picksettr_h
#define picksettr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "transl.h"
#include "bufstringset.h"

class BinIDValueSet;
class Conn;
class DataPointSet;
namespace Pick { class Set; }
template <class T> class ODPolygon;


mExpClass(Geometry) PickSetTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(PickSet);
    mODTextTranslationClass(PickSetTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(PickSet)

    const char*		defExtension() const		{ return "pck"; }
    static const char*	sKeyPickSet()			{ return "PickSet"; }
};


mExpClass(Geometry) PickSetTranslator : public Translator
{ mODTextTranslationClass(PickSetTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(PickSet)

    static bool		retrieve(Pick::Set&,const IOObj*,uiString&);
    static bool		store(const Pick::Set&,const IOObj*,uiString&);

    static bool		isPolygon(const IOObj&);
    static BufferString	getCategory(const IOObj&,Pick::Set* ps=0);

protected:

    virtual uiString	read(Pick::Set&,Conn&)				= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const Pick::Set&,Conn&)			= 0;
			//!< returns err msg or null on success
};


mExpClass(Geometry) dgbPickSetTranslator : public PickSetTranslator
{			     isTranslator(dgb,PickSet)
public:

			mDefEmptyTranslatorConstructor(dgb,PickSet)

protected:
    uiString		read(Pick::Set&,Conn&);
    uiString		write(const Pick::Set&,Conn&);

};


#endif
