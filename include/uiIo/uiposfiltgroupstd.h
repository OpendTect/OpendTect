#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiposfiltgroup.h"
class uiSpinBox;
class uiGenInput;


/*! \brief UI for Random Position filter */

mExpClass(uiIo) uiRandPosFiltGroup : public uiPosFiltGroup
{ mODTextTranslationClass(uiRandPosFiltGroup)
public:

			uiRandPosFiltGroup(uiParent*,const Setup&);

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    static uiPosFiltGroup* create( uiParent* p, const Setup& s )
    			{ return new uiRandPosFiltGroup(p,s); }
    static void		initClass();

protected:

    uiGenInput*		percpassfld_;

};


/*! \brief UI for Subsample Position filter */

mExpClass(uiIo) uiSubsampPosFiltGroup : public uiPosFiltGroup
{ mODTextTranslationClass(uiSubsampPosFiltGroup)
public:

			uiSubsampPosFiltGroup(uiParent*,const Setup&);

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    static uiPosFiltGroup* create( uiParent* p, const Setup& s )
    			{ return new uiSubsampPosFiltGroup(p,s); }
    static void		initClass();

protected:

    uiSpinBox*		eachfld_;

};


