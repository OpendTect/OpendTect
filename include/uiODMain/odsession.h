#ifndef odsession_h
#define odsession_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: odsession.h,v 1.2 2003-12-24 15:15:42 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "transl.h"
#include "iopar.h"

/*!\brief dTect session save/restore */

class ODSession
{
public:
    			ODSession()		{}
    virtual		~ODSession()		{}

    IOPar&		vispars() 		{ return vispars_; }
    IOPar&		attrpars() 		{ return attrpars_; }
    IOPar&		nlapars() 		{ return nlapars_; }
    IOPar&		scenepars() 		{ return scenepars_; }
    IOPar&		pluginpars() 		{ return pluginpars_; }

    void		clear();
    ODSession&		operator =(const ODSession&);
    bool		operator ==(const ODSession&) const;

    bool		usePar(const IOPar&);
    void		fillPar(IOPar& par) const;

protected:

    IOPar		vispars_;
    IOPar		scenepars_;
    IOPar		attrpars_;
    IOPar		nlapars_;
    IOPar		pluginpars_;

    static const char*	visprefix;
    static const char*	sceneprefix;
    static const char*	attrprefix;
    static const char*	nlaprefix;
    static const char*	pluginprefix;
};


class ODSessionTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ODSession)
public:
			mDefEmptyTranslatorGroupConstructor(ODSession)
    const char*		defExtension() const		{ return "sess"; }
};


class ODSessionTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(ODSession)

    virtual const char*	read(ODSession&,Conn&)		= 0;
    			//!< returns err msg or null on success
    virtual const char*	write(const ODSession&,Conn&)	= 0;
    			//!< returns err msg or null on success
    virtual const char*	warningMsg() const		{ return ""; }

    static bool		retrieve(ODSession&,const IOObj*,BufferString&);
    			//!< BufferString has errmsg, if any
    			//!< If true returned, errmsg contains warnings
    static bool		store(const ODSession&,const IOObj*,BufferString&);
    			//!< BufferString has errmsg, if any
    			//!< If true returned, errmsg contains warnings

    static const char*	keyword;

};
    

class dgbODSessionTranslator : public ODSessionTranslator
{				  isTranslator(dgb,ODSession)
public:
    			mDefEmptyTranslatorConstructor(dgb,ODSession)

    const char*		read(ODSession&,Conn&);
    			//!< returns err msg or null on success
    const char*		write( const ODSession&,Conn&);
    			//!< returns err msg or null on success
    const char*		warningMsg() const	{ return warningmsg; }

    BufferString	warningmsg;

};


#endif
