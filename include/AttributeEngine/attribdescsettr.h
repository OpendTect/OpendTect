#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id$
________________________________________________________________________

@$*/
 
#include "attributeenginemod.h"
#include "transl.h"
#include "ctxtioobj.h"
#include "uistring.h"

class Conn;
namespace Attrib { class DescSet; }

/*!
\brief Translator group for I/O of DescSet.
*/

mExpClass(AttributeEngine) AttribDescSetTranslatorGroup : public TranslatorGroup
{			  isTranslatorGroup(AttribDescSet)
public:
    			mDefEmptyTranslatorGroupConstructor(AttribDescSet)

    virtual const char*	defExtension() const		{ return "attr"; }
};


/*!
\brief Base Translator class for I/O of DescSet.
*/

mExpClass(AttributeEngine) AttribDescSetTranslator : public Translator
{ mODTextTranslationClass(AttribDescSetTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(AttribDescSet)

    virtual const char*	read(Attrib::DescSet&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	warningMsg() const			= 0;
    virtual const char*	write(const Attrib::DescSet&,Conn&)	= 0;
			//!< returns err msg or null on success

    static bool		retrieve(Attrib::DescSet&,const char* fnm,
				 uiString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static bool		retrieve(Attrib::DescSet&,const IOObj*,uiString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static bool 	store(const Attrib::DescSet&,const IOObj*,
			      uiString&);
			//!< BufferString has errmsg, if any
};


/*!
\brief Actual Translator class for I/O of DescSet.
*/

mExpClass(AttributeEngine)
dgbAttribDescSetTranslator : public AttribDescSetTranslator
{ isTranslator(dgb,AttribDescSet);
  mODTextTranslationClass(dgbAttribDescSetTranslator);
public:
			mDefEmptyTranslatorConstructor(dgb,AttribDescSet)
    const char*		read(Attrib::DescSet&,Conn&);
    const char* 	warningMsg() const {return warningmsg_.getFullString();}
    const char*		write(const Attrib::DescSet&,Conn&);

    uiString		warningmsg_;
};


