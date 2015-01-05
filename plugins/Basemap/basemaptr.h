#ifndef basemaptr_h
#define basemaptr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basemapmod.h"
#include "transl.h"

/*!\brief BaseMap session save/restore */

mExpClass(Basemap) BasemapTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(Basemap)
public:
			mDefEmptyTranslatorGroupConstructor(Basemap)

    const char*		defExtension() const	{ return "par"; }

};


mExpClass(Basemap) BasemapTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(Basemap)

    virtual const char* read(IOPar&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char* write(const IOPar&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(IOPar&,const IOObj*,BufferString&);
    static bool		store(const IOPar&,const IOObj*, BufferString&);
};


mExpClass(Basemap) odBasemapTranslator : public BasemapTranslator
{			     isTranslator(od,Basemap)
public:
			mDefEmptyTranslatorConstructor(od,Basemap)

    const char*		read(IOPar&,Conn&);
    const char*		write(const IOPar&,Conn&);

};


#endif

