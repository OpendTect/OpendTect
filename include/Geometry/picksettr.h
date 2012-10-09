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
 
#include "transl.h"
#include "bufstringset.h"
class Conn;
class BinIDValueSet;
class DataPointSet;
namespace Pick { class Set; }
template <class T> class ODPolygon;


mClass PickSetTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(PickSet)
public:
    			mDefEmptyTranslatorGroupConstructor(PickSet)

    const char*		defExtension() const		{ return "pck"; }
    static const char*	sKeyPickSet()			{ return "PickSet"; }
};


mClass PickSetTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(PickSet)

    virtual const char*	read(Pick::Set&,Conn&,bool checkdir=true)	= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const Pick::Set&,Conn&)			= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Pick::Set&,const IOObj*,bool checkdir,
	    			 BufferString&);
    static bool		store(const Pick::Set&,const IOObj*,BufferString&);

    static bool		getCoordSet(const char* ioobjkey,TypeSet<Coord3>&);
    			//!< Utility function
    static void		createBinIDValueSets(const BufferStringSet& ioobjids,
					     ObjectSet<BinIDValueSet>&);
    			//!< Utility function
    static void		createDataPointSets(const BufferStringSet&,
					     ObjectSet<DataPointSet>&,
					     bool is2d,bool mini=false);
    			//!< Utility function
    static ODPolygon<float>* getPolygon(const IOObj&,BufferString& errmsg);
    			//!< Returns null on failure
};


mClass dgbPickSetTranslator : public PickSetTranslator
{			     isTranslator(dgb,PickSet)
public:

    			mDefEmptyTranslatorConstructor(dgb,PickSet)

    const char*		read(Pick::Set&,Conn&,bool checkdir=true);
    const char*		write(const Pick::Set&,Conn&);

};


#endif
