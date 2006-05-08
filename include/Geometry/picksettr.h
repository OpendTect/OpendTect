#ifndef picksettr_h
#define picksettr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: picksettr.h,v 1.6 2006-05-08 16:50:19 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include "bufstringset.h"
class Conn;
namespace Pick { class SetGroup; }
class BinIDValueSet;


class PickSetGroupTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(PickSetGroup)
public:
    			mDefEmptyTranslatorGroupConstructor(PickSetGroup)

    const char*		defExtension() const		{ return "pck"; }
};


class PickSetGroupTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(PickSetGroup)

    virtual const char*	read(Pick::SetGroup&,Conn&,const bool* selarr=0) = 0;
			//!< returns err msg or null on success
    virtual const char*	write(const Pick::SetGroup&,Conn&,const bool* sa=0) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(Pick::SetGroup&,const IOObj*,BufferString&,
				const bool* selarr=0);
    static bool		store(const Pick::SetGroup&,const IOObj*,BufferString&,
				const bool* selarr=0,bool domrg=false);
    			//!< if domrg == true, if set already exists new set
			//!< will be merged

    static void		createBinIDValueSets(const BufferStringSet& ioobjids,
					     ObjectSet<BinIDValueSet>&);
    			//!< Utility function.
};


class dgbPickSetGroupTranslator : public PickSetGroupTranslator
{			     isTranslator(dgb,PickSetGroup)
public:

    			mDefEmptyTranslatorConstructor(dgb,PickSetGroup)

    const char*		read(Pick::SetGroup&,Conn&,const bool* s=0);
    const char*		write(const Pick::SetGroup&,Conn&,const bool* s=0);

};


#endif
