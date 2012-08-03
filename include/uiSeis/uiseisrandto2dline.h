#ifndef uiseisrandto2dline_h
#define uiseisrandto2dline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: uiseisrandto2dline.h,v 1.8 2012-08-03 13:01:08 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uigroup.h"

class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;

namespace Geometry { class RandomLine; class RandomLineSet; }

mClass(uiSeis) uiSeisRandTo2DBase : public uiGroup
{
public:
    			uiSeisRandTo2DBase(uiParent*,bool rdlsel);
			~uiSeisRandTo2DBase();

    const IOObj*	getInputIOObj() const;
    const IOObj*	getOutputIOObj() const;
    const char*		getAttribName() const;

    bool		getRandomLineGeom(Geometry::RandomLineSet&) const;

    virtual bool	checkInputs();

    Notifier<uiSeisRandTo2DBase> change;

protected:

    uiIOObjSel*		rdlfld_;
    uiSeisSel*		inpfld_;
    uiSeisSel*          outpfld_;

    void		selCB(CallBacker*);

};


mClass(uiSeis) uiSeisRandTo2DLineDlg : public uiDialog
{
public:
    			uiSeisRandTo2DLineDlg(uiParent*,
					      const Geometry::RandomLine*);

protected:

    uiSeisRandTo2DBase*	basegrp_;
    uiGenInput*		linenmfld_;
    uiGenInput*		trcnrfld_;
    const Geometry::RandomLine*	rdlgeom_;

    bool		acceptOK(CallBacker*);
};

#endif

