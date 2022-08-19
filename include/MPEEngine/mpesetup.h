#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

} // namespace MPE


typedef MPE::Setup MPESetup;

/*!
\brief TranslatorGroup for MPE::Setup.
*/

mExpClass(MPEEngine) MPESetupTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(MPESetup)
public:
			mDefEmptyTranslatorGroupConstructor(MPESetup)
    const char*		defExtension() const override		{ return "ts"; }
};


/*!
\brief Translator for MPE::Setup.
*/

mExpClass(MPEEngine) MPESetupTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(MPESetup)

    virtual const char* read(MPESetup&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char* write(const MPESetup&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char* warningMsg() const		{ return ""; }

    static bool		retrieve(MPESetup&,const IOObj*,BufferString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static bool		store(const MPESetup&,const IOObj*,BufferString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
};
    

/*!
\brief MPESetupTranslator for dgbMPESetup.
*/

mExpClass(MPEEngine) dgbMPESetupTranslator : public MPESetupTranslator
{				  isTranslator(dgb,MPESetup)
public:
			mDefEmptyTranslatorConstructor(dgb,MPESetup)

    const char*		read(MPESetup&,Conn&) override;
			//!< returns err msg or null on success
    const char*		write( const MPESetup&,Conn&) override;
			//!< returns err msg or null on success
    const char*		warningMsg() const override	{ return warningmsg; }

    BufferString	warningmsg;

};
