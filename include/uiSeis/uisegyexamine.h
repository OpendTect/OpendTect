#ifndef uisegyexamine_h
#define uisegyexamine_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id: uisegyexamine.h,v 1.1 2008-09-11 13:56:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class Timer;
class SeisTrc;
class uiTable;
class uiTextEdit;
class SeisTrcBuf;
class SEGYSeisTrcTranslator;


class uiSEGYExamine : public uiDialog
{
public:

    struct Setup : public uiDialog::Setup
    {
			Setup(int nrtraces=100);

	mDefSetupMemb(int,nrtrcs)
	mDefSetupMemb(StepInterval<int>,filenrs)
	mDefSetupMemb(int,nrzeropad)
	mDefSetupMemb(int,ns)			//!< overruling header
	mDefSetupMemb(int,fmt)			//!< overruling header
    };

			uiSEGYExamine(uiParent*,const char*,const Setup&);
			~uiSEGYExamine();

protected:

    void		onStartUp(CallBacker*);
    void		dispSeis(CallBacker*);
    void		updateInput(CallBacker*);
    void		vwrClose(CallBacker*);

    void		updateInp();
    void		handleFirstTrace(const SeisTrc&,
	    				 const SEGYSeisTrcTranslator&);
    bool		rejectOK();

    void		outInfo(const char*);

    Setup		setup_;
    BufferString	txtinfo_;
    BufferString	fname_;
    SeisTrcBuf&		tbuf_;
    Timer&		timer_;

    uiTextEdit*		txtfld_;
    uiTable*		tbl_;
};


#endif
