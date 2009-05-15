#ifndef uichangesurfacedlg_h
#define uichangesurfacedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          June 2006
 RCS:           $Id: uichangesurfacedlg.h,v 1.9 2009-05-15 17:58:54 cvskris Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

namespace EM { class Horizon3D; }

class Executor;
class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
template <class T> class Array2D;
template <class T> class StepInterval;

/*!\brief Base class for surface changers. At the moment only does horizons. */

mClass uiChangeSurfaceDlg : public uiDialog
{
public:
				uiChangeSurfaceDlg(uiParent*,EM::Horizon3D*,
						   const char*);
				~uiChangeSurfaceDlg();

protected:

    uiIOObjSel*			inputfld_;
    uiGenInput*			savefld_;
    uiIOObjSel*			outputfld_;
    uiGroup*			parsgrp_;

    EM::Horizon3D*		horizon_;
    CtxtIOObj*			ctioin_;
    CtxtIOObj*			ctioout_;

    void			saveCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			readHorizon();
    bool			saveHorizon();
    bool			doProcessing();

    void			attachPars();	//!< To be called by subclass
    virtual const char*		infoMsg(const Executor*) const	{ return 0; }
    virtual Executor*		getWorker(Array2D<float>&,
					  const StepInterval<int>&,
					  const StepInterval<int>&) = 0;
    virtual bool		fillUdfsOnly() const		{ return false;}
    virtual bool		needsFullSurveyArray() const	{ return false;}
    virtual const char*		undoText() const		{ return 0; }
};




class uiStepOutSel;

mClass uiFilterHorizonDlg : public uiChangeSurfaceDlg
{
public:
				uiFilterHorizonDlg(uiParent*,EM::Horizon3D*);

protected:

    uiGenInput*			medianfld_;
    uiStepOutSel*		stepoutfld_;

    Executor*			getWorker(Array2D<float>&,
	    				  const StepInterval<int>&,
					  const StepInterval<int>&);
    virtual const char*		undoText() const { return "filtering"; }

};


#endif
