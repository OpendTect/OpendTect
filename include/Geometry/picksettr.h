#ifndef picksettr_h
#define picksettr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: picksettr.h,v 1.5 2004-07-16 15:35:25 bert Exp $
________________________________________________________________________

-*/
 
#include "transl.h"
#include "bufstringset.h"
class Conn;
class PickSetGroup;
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

    virtual const char*	read(PickSetGroup&,Conn&,const bool* selarr=0) = 0;
			//!< returns err msg or null on success
    virtual const char*	write(const PickSetGroup&,Conn&,const bool* sa=0) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(PickSetGroup&,const IOObj*,BufferString&,
				const bool* selarr=0);
    static bool		store(const PickSetGroup&,const IOObj*,BufferString&,
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

    const char*		read(PickSetGroup&,Conn&,const bool* s=0);
    const char*		write(const PickSetGroup&,Conn&,const bool* s=0);

};


#endif
