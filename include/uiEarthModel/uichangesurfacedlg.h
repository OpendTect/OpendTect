#ifndef uichangesurfacedlg_h
#define uichangesurfacedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2006
 RCS:           $Id: uichangesurfacedlg.h,v 1.11 2009-11-04 16:01:05 cvsyuancheng Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

namespace EM { class Horizon3D; }

class uiHorSaveFieldGrp;
class Executor;
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
    uiHorSaveFieldGrp*		saveFldGrp() const { return savefldgrp_; }

protected:

    uiHorSaveFieldGrp*		savefldgrp_;				
    uiIOObjSel*			inputfld_;
    uiGroup*			parsgrp_;

    EM::Horizon3D*		horizon_;

    bool			acceptOK(CallBacker*);
    bool			readHorizon();
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
