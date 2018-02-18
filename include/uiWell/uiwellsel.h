#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "dbkey.h"
#include "welldata.h"

class uiWellSingLineMultiSel;


mExpClass(uiWell) uiWellSel : public uiIOObjSel
{ mODTextTranslationClass(uiWellSel)
public:

			uiWellSel(uiParent*,bool forread,const Setup&);
			uiWellSel(uiParent*,bool forread,
				const uiString& seltxt=uiString::empty(),
				bool withinserters=true);

    ConstRefMan<Well::Data>	getWellData() const;
    RefMan<Well::Data>		getWellDataForEdit() const;

protected:

    Setup		getSetup(bool,const uiString&,bool) const;
    IOObjContext	getContext(bool,bool) const;

};


mExpClass(uiWell) uiMultiWellSel : public uiGroup
{ mODTextTranslationClass(uiMultiWellSel);
public:

			uiMultiWellSel(uiParent*,bool single_line,
					const uiIOObjSelGrp::Setup* su=0);

    int			nrWells() const;
    int			nrSelected() const;
    void		setSelected(const DBKeySet&);
    void		getSelected(DBKeySet&) const;
    DBKey		currentID() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    Notifier<uiMultiWellSel>	newSelection;
    Notifier<uiMultiWellSel>	newCurrent;

protected:

    uiWellSingLineMultiSel*	singlnfld_;
    uiIOObjSelGrp*		multilnfld_;

    void    newSelectionCB( CallBacker* )   { newSelection.trigger(); }
    void    newCurrentCB( CallBacker* )	    { newCurrent.trigger(); }

};
