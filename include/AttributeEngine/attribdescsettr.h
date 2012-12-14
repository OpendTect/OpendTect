#ifndef attribdescsettr_h
#define attribdescsettr_h

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
class Conn;
namespace Attrib { class DescSet; }

/*!
  \ingroup AttributeEngine
  \brief Translator group for I/O of DescSet.
*/

mClass(AttributeEngine) AttribDescSetTranslatorGroup : public TranslatorGroup
{			  isTranslatorGroup(AttribDescSet)
public:
    			mDefEmptyTranslatorGroupConstructor(AttribDescSet)

    virtual const char*	defExtension() const		{ return "attr"; }
};


/*!
  \ingroup AttributeEngine
  \brief Base Translator class for I/O of DescSet.
*/

mClass(AttributeEngine) AttribDescSetTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(AttribDescSet)

    virtual const char*	read(Attrib::DescSet&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char*	warningMsg() const		= 0;
    virtual const char*	write(const Attrib::DescSet&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(Attrib::DescSet&,const IOObj*,BufferString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static bool		store(const Attrib::DescSet&,const IOObj*,BufferString&);
			//!< BufferString has errmsg, if any
};


/*!
  \ingroup AttributeEngine
  \brief Actual Translator class for I/O of DescSet.
*/
mClass(AttributeEngine) dgbAttribDescSetTranslator : public AttribDescSetTranslator
{			     isTranslator(dgb,AttribDescSet)
public:
			mDefEmptyTranslatorConstructor(dgb,AttribDescSet)

    const char*		read(Attrib::DescSet&,Conn&);
    const char*		warningMsg() const		  { return warningmsg; }
    const char*		write(const Attrib::DescSet&,Conn&);

    BufferString	warningmsg;
};


#endif

