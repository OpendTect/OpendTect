#ifndef mpesetup_h
#define mpesetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		March 2004
 RCS:		$Id$
________________________________________________________________________


-*/

#include "mpeenginemod.h"
#include "transl.h"

class IOPar;

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
{				    isTranslatorGroup(MPESetup)
public:
			mDefEmptyTranslatorGroupConstructor(MPESetup)
    const char*		defExtension() const		{ return "ts"; }
};


/*!
\brief Translator for MPE::Setup.
*/

mExpClass(MPEEngine) MPESetupTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(MPESetup)

    virtual const char*	read(MPESetup&,Conn&)		= 0;
    			//!< returns err msg or null on success
    virtual const char*	write(const MPESetup&,Conn&)	= 0;
    			//!< returns err msg or null on success
    virtual const char*	warningMsg() const		{ return ""; }

    static bool		retrieve(MPESetup&,const IOObj*,BufferString&);
    			//!< BufferString has errmsg, if any
    			//!< If true returned, errmsg contains warnings
    static bool		store(const MPESetup&,const IOObj*,BufferString&);
    			//!< BufferString has errmsg, if any
    			//!< If true returned, errmsg contains warnings

    static const char*	keyword;

};
    

/*!
\brief MPESetupTranslator for dgbMPESetup.
*/

mExpClass(MPEEngine) dgbMPESetupTranslator : public MPESetupTranslator
{				  isTranslator(dgb,MPESetup)
public:
    			mDefEmptyTranslatorConstructor(dgb,MPESetup)

    const char*		read(MPESetup&,Conn&);
    			//!< returns err msg or null on success
    const char*		write( const MPESetup&,Conn&);
    			//!< returns err msg or null on success
    const char*		warningMsg() const	{ return warningmsg; }

    BufferString	warningmsg;

};

#endif

