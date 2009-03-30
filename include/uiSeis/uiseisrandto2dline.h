#ifndef uiseisrandto2dline_h
#define uiseisrandto2dline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: uiseisrandto2dline.h,v 1.3 2009-03-30 06:58:43 cvsraman Exp $
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
class uiPushButton;
class uiSeisSel;
class uiWorld2Ui;

namespace Geometry { class RandomLine; class RandomLineSet; }

mClass uiSeisRandTo2DBase : public uiGroup
{
public:
    			uiSeisRandTo2DBase(uiParent*,
					   const Geometry::RandomLine&);
			~uiSeisRandTo2DBase();

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    const Geometry::RandomLine& randln_;

    uiSeisSel*		inpfld_;
    uiSeisSel*          outpfld_;

    virtual bool	checkInputs();

};


mClass uiSeisRandTo2DLineDlg : public uiDialog
{
public:
    			uiSeisRandTo2DLineDlg(uiParent*,
					      const Geometry::RandomLine&);

protected:

    uiSeisRandTo2DBase*	basegrp_;
    uiGenInput*		linenmfld_;
    uiGenInput*		trcnrfld_;

    bool		acceptOK(CallBacker*);
};


mClass uiSeisRandTo2DGridDlg : public uiFullBatchDialog
{
public:
    			uiSeisRandTo2DGridDlg(uiParent*,
					      const Geometry::RandomLine&);

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

    Geometry::RandomLineSet*	parallelset_;
    Geometry::RandomLineSet*	perpset_;

    void		distChgCB(CallBacker*);

    bool		checkInputs();
    bool		createLines();
    void		createPreview();
    void		updatePreview();
    void		drawLines(const uiWorld2Ui&,bool);
};

#endif
