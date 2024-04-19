#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
{				    isTranslatorGroup(PickSet)
public:
			mDefEmptyTranslatorGroupConstructor(PickSet)

    const char*		defExtension() const override	{ return "pck"; }
    static const char*	sKeyPickSet()			{ return "PickSet"; }
};


mExpClass(Geometry) PickSetTranslator : public Translator
{ mODTextTranslationClass(PickSetTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(PickSet)

    virtual uiString	read(Pick::Set&,Conn&,bool checkdir=true)	= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const Pick::Set&,Conn&)			= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Pick::Set&,const IOObj*,bool checkdir,
				 uiString&);
    static bool		store(const Pick::Set&,const IOObj*,uiString&);

    static bool		getCoordSet(const char* ioobjkey,TypeSet<Coord3>&);
			//!< Do not use, will be removed after 6.0
    static bool		getCoordSet(const char* ioobjkey,TypeSet<Coord3>&,
				    TypeSet<TrcKey>&);
			//!< Utility function
    static void		createBinIDValueSets(const BufferStringSet& ioobjids,
					     ObjectSet<BinIDValueSet>&);
			//!< Utility function
    static void		createDataPointSets(const BufferStringSet&,
					     RefObjectSet<DataPointSet>&,
					     bool is2d,bool mini=false);
			//!< Utility function
    static ODPolygon<float>* getPolygon(const IOObj&,uiString& errmsg);
			//!< Returns null on failure

    static void		fillConstraints(IOObjContext&,bool ispoly);
    static void		tagLegacyPickSets();

    bool		implRemove(const IOObj*,bool) const override;
    bool		implRename(const IOObj*,const char*) const override;
};


mExpClass(Geometry) dgbPickSetTranslator : public PickSetTranslator
{ mODTextTranslationClass(dgbPickSetTranslator)
			isTranslator(dgb,PickSet)
public:

			mDefEmptyTranslatorConstructor(dgb,PickSet)

    uiString		read(Pick::Set&,Conn&,bool checkdir=true) override;
    uiString		write(const Pick::Set&,Conn&) override;

};


namespace Pick
{
mGlobal(Geometry) RefMan<Pick::Set>	getSet(const MultiID&,uiString&);
mGlobal(Geometry) RefMan<Pick::Set>	getSet(const DBKey&,uiString&);
}
