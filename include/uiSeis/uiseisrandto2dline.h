#ifndef uiseisrandto2dline_h
#define uiseisrandto2dline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: uiseisrandto2dline.h,v 1.5 2010-06-10 08:26:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "uigroup.h"
#include "uilabel.h"
#include "position.h"

class CtxtIOObj;
class IOObj;
class LineKey;
class uiGenInput;
class uiGraphicsView;
class uiIOObjSel;
class uiPushButton;
class uiSeisSel;
class uiWorld2Ui;

namespace Geometry { class RandomLine; class RandomLineSet; }

mClass uiSeisRandTo2DBase : public uiGroup
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


mClass uiSeisRandTo2DLineDlg : public uiDialog
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


mClass uiSeisRandTo2DGridDlg : public uiFullBatchDialog
{
public:
    			uiSeisRandTo2DGridDlg(uiParent*,
					      const Geometry::RandomLine*);

    bool		fillPar(IOPar&);
    bool		prepareProcessing()		{ return true; }

protected:

    uiSeisRandTo2DBase*	basegrp_;
    uiGroup*		inpgrp_;
    uiGenInput*		parlineprefixfld_;
    uiGenInput*		perplineprefixfld_;
    uiGenInput*		distfld_;
    uiLabel*		nrparlinesfld_;
    uiLabel*		nrperplinesfld_;
    uiGraphicsView*	preview_;

    const Geometry::RandomLine*	rdlgeom_;
    Geometry::RandomLineSet*	parallelset_;
    Geometry::RandomLineSet*	perpset_;

    void		distChgCB(CallBacker*);

    bool		checkInputs();
    bool		createLines();
    void		createPreview();
    void		updatePreview();
    void		drawLines(const uiWorld2Ui&,bool);

    bool		getNodePositions(BinID&,BinID&) const;
};

#endif
