#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001
________________________________________________________________________

-*/

#include "attributeenginecommon.h"
#include "transl.h"
#include "ioobjctxt.h"
#include "uistring.h"

class Conn;

/*!\brief Translator group for I/O of DescSet. */

mExpClass(AttributeEngine) AttribDescSetTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(AttribDescSet);
    mODTextTranslationClass(AttribDescSetTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(AttribDescSet)

    virtual const char*	defExtension() const		{ return "attr"; }
};


/*!\brief Base Translator class for I/O of DescSet. */

mExpClass(AttributeEngine) AttribDescSetTranslator : public Translator
{ mODTextTranslationClass(AttribDescSetTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(AttribDescSet)

    virtual uiRetVal	read(Attrib::DescSet&,Conn&)		= 0;
    virtual uiRetVal	warnings() const			= 0;
    virtual uiRetVal	write(const Attrib::DescSet&,Conn&)	= 0;

    static uiRetVal	retrieve(Attrib::DescSet&,const char* fnm,
				 uiRetVal* warnings=0);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static uiRetVal	retrieve(Attrib::DescSet&,const IOObj*,
				 uiRetVal* warnings=0);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static uiRetVal	store(const Attrib::DescSet&,const IOObj*,
			      uiRetVal* warnings=0);
			//!< BufferString has errmsg, if any
    static uiRetVal	readFromStream(ascistream&,Attrib::DescSet&,
					uiRetVal& warns);
};


/*!\brief Actual Translator class for I/O of DescSet. */

mExpClass(AttributeEngine)
dgbAttribDescSetTranslator : public AttribDescSetTranslator
{ isTranslator(dgb,AttribDescSet);
  mODTextTranslationClass(dgbAttribDescSetTranslator);
public:
			mDefEmptyTranslatorConstructor(dgb,AttribDescSet)

    uiRetVal		read(Attrib::DescSet&,Conn&);
    uiRetVal		warnings() const	    { return warns_;}
    uiRetVal		write(const Attrib::DescSet&,Conn&);

    uiRetVal		warns_;

protected:

    uiRetVal		badConnRV();
};
