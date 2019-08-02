#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "transl.h"

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

    static uiRetVal	retrieve(Pick::Set&,const IOObj&);
    static uiRetVal	store(const Pick::Set&,const IOObj&);

    static bool		isPolygon(const IOObj&);
    static BufferString	getCategory(const IOObj&,Pick::Set* ps=0);

protected:

    virtual uiRetVal	read(Pick::Set&,const IOObj&)		= 0;
			//!< returns err msg or null on success
    virtual uiRetVal	write(const Pick::Set&,const IOObj&)	= 0;
			//!< returns err msg or null on success
};


mExpClass(Geometry) dgbPickSetTranslator : public PickSetTranslator
{			     isTranslator(dgb,PickSet)
public:

			mDefEmptyTranslatorConstructor(dgb,PickSet)

    const char*		defExtension() const;

protected:

    uiRetVal		read(Pick::Set&,const IOObj&);
    uiRetVal		write(const Pick::Set&,const IOObj&);

};
