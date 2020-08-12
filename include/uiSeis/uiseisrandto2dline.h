#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "uistring.h"

class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiSeis2DLineNameSel;

namespace Geometry { class RandomLine; class RandomLineSet; }

mExpClass(uiSeis) uiSeisRandTo2DBase : public uiGroup
{ mODTextTranslationClass(uiSeisRandTo2DBase);
public:
    			uiSeisRandTo2DBase(uiParent*,bool rdlsel);
			~uiSeisRandTo2DBase();

    const IOObj*	getInputIOObj() const;
    const IOObj*	getOutputIOObj() const;

    bool		getRandomLineGeom(Geometry::RandomLineSet&) const;

    virtual bool	checkInputs();

    Notifier<uiSeisRandTo2DBase> change;

protected:

    uiIOObjSel*		rdlfld_;
    uiSeisSel*		inpfld_;
    uiSeisSel*          outpfld_;

    void		selCB(CallBacker*);

};


mExpClass(uiSeis) uiSeisRandTo2DLineDlg : public uiDialog
{mODTextTranslationClass(uiSeisRandTo2DLineDlg)
public:
				uiSeisRandTo2DLineDlg(uiParent*,
					      const Geometry::RandomLine*);

protected:

    uiSeisRandTo2DBase*		basegrp_;
    uiSeis2DLineNameSel*	linenmfld_;
    uiGenInput*			trcnrfld_;
    const Geometry::RandomLine*	rdlgeom_;

    bool			acceptOK(CallBacker*);
};

