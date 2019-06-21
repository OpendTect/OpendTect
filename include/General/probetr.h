#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "transl.h"

class Probe;

mExpClass(General) ProbeTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(Probe);
    mODTextTranslationClass(ProbeTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(Probe)

    const char*		defExtension() const		{ return "probe"; }
    static const char*	sKeyProbe()			{ return "Probe"; }
};


mExpClass(General) ProbeTranslator : public Translator
{ mODTextTranslationClass(ProbeTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(Probe)

    static Probe*	retrieve(const IOObj*,uiString&);
    static bool		store(const Probe&,const IOObj*,uiString&);

protected:

    virtual Probe*	read(Conn&,uiString&)				= 0;
			//!< returns err msg or null on success
    virtual uiString	write(const Probe&,Conn&)			= 0;
			//!< returns err msg or null on success
};


mExpClass(General) dgbProbeTranslator : public ProbeTranslator
{			     isTranslator(dgb,Probe)
public:

			mDefEmptyTranslatorConstructor(dgb,Probe)

protected:

    Probe*		read(Conn&,uiString&);
    uiString		write(const Probe&,Conn&);

};
