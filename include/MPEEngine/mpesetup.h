#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		March 2004
________________________________________________________________________


-*/

#include "mpeenginemod.h"
#include "transl.h"


namespace MPE {

/*!
\brief MPE Setup read/save.
*/

mExpClass(MPEEngine) Setup
{
public:
				Setup();
				~Setup();

    bool			usePar(const IOPar&);
    void			fillPar(IOPar& par) const;

protected:
    IOPar&			pars;

protected:

};

}; // namespace MPE


typedef MPE::Setup MPESetup;

/*!
\brief TranslatorGroup for MPE::Setup.
*/

mExpClass(MPEEngine) MPESetupTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(MPESetup);
    mODTextTranslationClass(MPESetupTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(MPESetup)
    const char*		defExtension() const		{ return "ts"; }
};


/*!
\brief Translator for MPE::Setup.
*/

mExpClass(MPEEngine) MPESetupTranslator : public Translator
{ mODTextTranslationClass(MPESetupTranslator)
public:
			mDefEmptyTranslatorBaseConstructor(MPESetup)

    virtual uiString	read(MPESetup&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const MPESetup&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual uiString	warningMsg() const	{ return uiString::empty(); }

    static bool		retrieve(MPESetup&,const IOObj*,BufferString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static bool		retrieve(MPESetup&,const IOObj*,uiString&);
    static bool		store(const MPESetup&,const IOObj*,BufferString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
};


/*!
\brief MPESetupTranslator for dgbMPESetup.
*/

mExpClass(MPEEngine) dgbMPESetupTranslator : public MPESetupTranslator
{ mODTextTranslationClass(dgbMPESetupTranslator)
  isTranslator(dgb,MPESetup)
public:
			mDefEmptyTranslatorConstructor(dgb,MPESetup)

    uiString		read(MPESetup&,Conn&);
    uiString		write( const MPESetup&,Conn&);
    uiString		warningMsg() const	{ return warningmsg; }

    uiString	warningmsg;

};
