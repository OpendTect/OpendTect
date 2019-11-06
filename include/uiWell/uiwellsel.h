#ifndef uiwellsel_h
#define uiwellsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uicompoundparsel.h"
#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "multiid.h"


mExpClass(uiWell) uiWellSel : public uiIOObjSel
{ mODTextTranslationClass(uiWellSel)
public:

			uiWellSel(uiParent*,bool forread,const Setup&);
			uiWellSel(uiParent*,bool forread,
				const uiString& seltxt=uiString::emptyString(),
				bool withinserters=true);

protected:

    Setup		getSetup(bool,const uiString&,bool) const;
    IOObjContext	getContext(bool,bool) const;

};


mExpClass(uiWell) uiWellParSel : public uiCompoundParSel
{ mODTextTranslationClass(uiWellParSel);
public:
			uiWellParSel(uiParent*);
			~uiWellParSel();

    int			nrSelected() const	{ return selids_.size(); }
    void		setSelected(const TypeSet<MultiID>&);
    void		getSelected(TypeSet<MultiID>&) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    Notifier<uiWellParSel>	selDone;

protected:

    void		doDlg(CallBacker*);
    BufferString	getSummary() const;

    TypeSet<MultiID>	selids_;
    IOPar&		iopar_;
};


mExpClass(uiWell) uiMultiWellSel : public uiGroup
{ mODTextTranslationClass(uiMultiWellSel);
public:
			uiMultiWellSel(uiParent*,bool single_line,
					const uiIOObjSelGrp::Setup* su=0);

    int			nrSelected() const;
    void		setSelected(const TypeSet<MultiID>&);
    void		getSelected(TypeSet<MultiID>&) const;
    MultiID		currentID() const;
    void		allowIOObjManip(bool yn);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    Notifier<uiMultiWellSel>	newSelection;
    Notifier<uiMultiWellSel>	newCurrent;

protected:

    uiWellParSel*	singlnfld_;
    uiIOObjSelGrp*	multilnfld_;

    void		newSelectionCB(CallBacker*)  { newSelection.trigger(); }
    void		newCurrentCB(CallBacker*)    { newCurrent.trigger(); }

};

#endif
