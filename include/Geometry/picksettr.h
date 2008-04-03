#ifndef picksettr_h
#define picksettr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: picksettr.h,v 1.9 2008-04-03 11:18:47 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include "bufstringset.h"
class Conn;
class BinIDValueSet;
class DataPointSet;
namespace Pick { class Set; }
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


class dgbPickSetTranslator : public PickSetTranslator
{			     isTranslator(dgb,PickSet)
public:

    			mDefEmptyTranslatorConstructor(dgb,PickSet)

    const char*		read(Pick::Set&,Conn&);
    const char*		write(const Pick::Set&,Conn&);

};


#endif
