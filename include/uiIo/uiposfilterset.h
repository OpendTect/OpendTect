#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uicompoundparsel.h"
#include "uiposfiltgroup.h"
#include "iopar.h"
class uiGenInput;
class uiListBox;

/*! \brief lets user choose a way to provide positions */

mExpClass(uiIo) uiPosFilterSet : public uiGroup
{ mODTextTranslationClass(uiPosFilterSet);
public:

    struct Setup : public uiPosFiltGroup::Setup
    {
			Setup( bool is_2d )
			    : uiPosFiltGroup::Setup(is_2d)
			    , seltxt_(uiStrings::sFilters())
			    , incprovs_(false)		{}

	mDefSetupMemb(uiString,seltxt)
	mDefSetupMemb(bool,incprovs)
    };

    			uiPosFilterSet(uiParent*,const Setup&);

    void		usePar(const IOPar&);
    bool		fillPar(IOPar&) const;

protected:

    Setup			setup_;
    BoolTypeSet			issel_;
    BoolTypeSet			isprov_;

    uiGenInput*			ynfld_;
    ObjectSet<uiPosFiltGroup>	grps_;
    uiListBox*			selfld_;

    void			selChg(CallBacker*);
    int				selIdx() const;
};


mExpClass(uiIo) uiPosFilterSetSel : public uiCompoundParSel
{
public:

    typedef uiPosFilterSet::Setup Setup;

    			uiPosFilterSetSel(uiParent*,const Setup&);

    void		usePar(const IOPar&);
    void		fillPar( IOPar& iop ) const	{ iop.merge(iop_); }

protected:

    Setup		setup_;
    IOPar		iop_;

    void		doDlg(CallBacker*);
    BufferString	getSummary() const override;

};
