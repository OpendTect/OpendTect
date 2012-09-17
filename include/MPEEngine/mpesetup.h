#ifndef mpesetup_h
#define mpesetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		March 2004
 RCS:		$Id: mpesetup.h,v 1.3 2009/07/22 16:01:16 cvsbert Exp $
________________________________________________________________________


-*/

#include "transl.h"

class IOPar;


/*!\brief MPE Setup read/save */

namespace MPE {

mClass Setup
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

mClass MPESetupTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(MPESetup)
public:
			mDefEmptyTranslatorGroupConstructor(MPESetup)
    const char*		defExtension() const		{ return "ts"; }
};


mClass MPESetupTranslator : public Translator
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
    

mClass dgbMPESetupTranslator : public MPESetupTranslator
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
