#ifndef uiposfilterset_h
#define uiposfilterset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposfilterset.h,v 1.5 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uiposfiltgroup.h"
#include "iopar.h"
class uiGenInput;
class uiListBox;

/*! \brief lets user choose a way to provide positions */

mClass uiPosFilterSet : public uiGroup
{
public:

    struct Setup : public uiPosFiltGroup::Setup
    {
			Setup( bool is_2d )
			    : uiPosFiltGroup::Setup(is_2d)
			    , seltxt_("Filters")
			    , incprovs_(false)		{}

	mDefSetupMemb(BufferString,seltxt)
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


mClass uiPosFilterSetSel : public uiCompoundParSel
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
    BufferString	getSummary() const;

};


#endif
