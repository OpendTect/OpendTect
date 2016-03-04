#ifndef picksettr_h
#define picksettr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id$
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "transl.h"
#include "bufstringset.h"
class Conn;
class BinIDValueSet;
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

    virtual uiString	read(Pick::Set&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const Pick::Set&,Conn&)			= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Pick::Set&,const IOObj*,
				 BufferString&);
    static bool		retrieve(Pick::Set&,const IOObj*,
				 uiString&);
    static bool		store(const Pick::Set&,const IOObj*,BufferString&);
    static bool		store(const Pick::Set&,const IOObj*,uiString&);

    static bool		getCoordSet(const char* ioobjkey,TypeSet<Coord3>&,
				    TypeSet<TrcKey>&,uiString& errmsg);
			//!< Utility function
    static void		createBinIDValueSets(const BufferStringSet& ioobjids,
					     ObjectSet<BinIDValueSet>&,
					     uiString& errmsg);
			//!< Utility function
    static void		createDataPointSets(const BufferStringSet&,
					     ObjectSet<DataPointSet>&,
					     uiString& errmsg,
					     bool is2d,bool mini=false);
			//!< Utility function
    static ODPolygon<float>* getPolygon(const IOObj&,uiString& errmsg);
			//!< Returns null on failure
};


mExpClass(Geometry) dgbPickSetTranslator : public PickSetTranslator
{			     isTranslator(dgb,PickSet)
public:

			mDefEmptyTranslatorConstructor(dgb,PickSet)

    uiString		read(Pick::Set&,Conn&);
    uiString		write(const Pick::Set&,Conn&);

};


#endif
