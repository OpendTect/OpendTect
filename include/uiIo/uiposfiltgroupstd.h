#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiRandPosFiltGroup();

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
			~uiSubsampPosFiltGroup();

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    static uiPosFiltGroup* create( uiParent* p, const Setup& s )
    			{ return new uiSubsampPosFiltGroup(p,s); }
    static void		initClass();

protected:

    uiSpinBox*		eachfld_;

};
