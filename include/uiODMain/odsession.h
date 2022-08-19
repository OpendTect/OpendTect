#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "sets.h"
#include "transl.h"
#include "iopar.h"

/*!\brief dTect session save/restore */

mExpClass(uiODMain) ODSession
{ mODTextTranslationClass(ODSession);
public:
			ODSession();
    virtual		~ODSession()		{}

    IOPar&		empars()		{ return empars_; }
    IOPar&		seispars()		{ return seispars_; }
    IOPar&		vispars()		{ return vispars_; }
    IOPar&		attrpars(bool,bool);
    IOPar&		nlapars()		{ return nlapars_; }
    IOPar&		mpepars()		{ return mpepars_; }
    IOPar&		scenepars()		{ return scenepars_; }
    IOPar&		vwr2dpars()		{ return vwr2dpars_; }
    IOPar&		pluginpars()		{ return pluginpars_; }

    void		clear();
    ODSession&		operator =(const ODSession&);
    bool		operator ==(const ODSession&) const;

    bool		usePar(const IOPar&);
    void		fillPar(IOPar& par) const;

    static void		getStartupData(bool& douse,MultiID&);
    static void		setStartupData(bool,const MultiID&);

protected:

    IOPar		empars_;
    IOPar		seispars_;
    IOPar		vispars_;
    IOPar		scenepars_;
    IOPar		attrpars_;
    IOPar		attrpars2d_;
    IOPar		attrpars3d_;
    IOPar		attrpars2dstored_;
    IOPar		attrpars3dstored_;
    IOPar		nlapars_;
    IOPar		mpepars_;
    IOPar		pluginpars_;
    IOPar		vwr2dpars_;

    static const char*	emprefix();
    static const char*	seisprefix();
    static const char*	visprefix();
    static const char*	sceneprefix();
    static const char*	attrprefix();
    static const char*	attr2dprefix();
    static const char*	attr3dprefix();
    static const char*	attr2dstoredprefix();
    static const char*	attr3dstoredprefix();
    static const char*	nlaprefix();
    static const char*	trackprefix();
    static const char*	vwr2dprefix();
    static const char*	pluginprefix();

    static const char*	sKeyUseStartup();
    static const char*	sKeyStartupID();
};


mExpClass(uiODMain) ODSessionTranslatorGroup : public TranslatorGroup
{				    isTranslatorGroup(ODSession)
public:
			mDefEmptyTranslatorGroupConstructor(ODSession)
    const char*		defExtension() const override	{ return "sess"; }
};


mExpClass(uiODMain) ODSessionTranslator : public Translator
{ mODTextTranslationClass(ODSessionTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(ODSession)

    virtual const char*	read(ODSession&,Conn&)		= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const ODSession&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char*	warningMsg() const		{ return ""; }

    static bool		retrieve(ODSession&,const IOObj*,uiString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
    static bool		store(const ODSession&,const IOObj*,uiString&);
			//!< BufferString has errmsg, if any
			//!< If true returned, errmsg contains warnings
};


mExpClass(uiODMain) dgbODSessionTranslator : public ODSessionTranslator
{				  isTranslator(dgb,ODSession)
public:
			mDefEmptyTranslatorConstructor(dgb,ODSession)

    const char*		read(ODSession&,Conn&) override;
			//!< returns err msg or null on success
    const char*		write( const ODSession&,Conn&) override;
			//!< returns err msg or null on success
    const char*		warningMsg() const override	{ return warningmsg; }

    BufferString	warningmsg;

};


#include "uiobjfileman.h"


/*! \brief
Session manager
*/

mExpClass(uiODMain) uiSessionMan : public uiObjFileMan
{ mODTextTranslationClass(ODSessionTranslator);
public:
				uiSessionMan(uiParent*);
				~uiSessionMan();

    mDeclInstanceCreatedNotifierAccess(uiSessionMan);

protected:

    void			mkFileInfo();
};
