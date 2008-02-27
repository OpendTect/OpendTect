#ifndef uiposfilterset_h
#define uiposfilterset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposfilterset.h,v 1.2 2008-02-27 17:27:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uiposfiltgroup.h"
#include "iopar.h"
class uiGenInput;
class uiListBox;

/*! \brief lets user choose a way to provide positions */

class uiPosFilterSet : public uiGroup
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

    void			ynChg(CallBacker*);
    void			selChg(CallBacker*);
    int				selIdx() const;
};


class uiPosFilterSetSel : public uiCompoundParSel
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
