#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"
#include "segyfiledef.h"

class SEGYSeisTrcTranslator;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcReader;
class Timer;
class uiSEGYTrcHdrValPlot;
class uiSpinBox;
class uiTable;
class uiTextEdit;


/* The dialog for examining SEG-Y files */

mExpClass(uiSEGY) uiSEGYExamine : public uiDialog
{ mODTextTranslationClass(uiSEGYExamine);
public:

    mStruct(uiSEGY) Setup : public uiDialog::Setup
    {
				Setup(int nrtraces=100);

	mDefSetupMemb(int,nrtrcs)
	mDefSetupMemb(SEGY::FileSpec,fs)
	mDefSetupMemb(FilePars,fp)

	void			usePar(const IOPar&);
	static const char*	sKeyNrTrcs;
    };

			uiSEGYExamine(uiParent*,const Setup&);
			~uiSEGYExamine();

    int			getRev() const;		//!< -1 = err, 1 = Rev 1
    const char*		errMsg() const		{ return txtinfo_; }

    static SeisTrcReader* getReader(const Setup&,BufferString& errmsg);
    static int		getRev(const SeisTrcReader&); // -1 = err, 1 = Rev 1
    static int		getRev(const Setup&,BufferString& emsg);

    static bool		launch(const Setup&);

protected:

    Setup		setup_;
    SeisTrcReader*	rdr_;
    BufferString	txtinfo_;
    BufferString	fname_;
    SeisTrcBuf&		tbuf_;
    Timer*		timer_;

    uiTextEdit*		txtfld_;
    uiTable*		tbl_;
    uiSpinBox*		trc0fld_;
    uiSpinBox*		stepfld_;
    uiSpinBox*		nrtrcsfld_;
    uiSEGYTrcHdrValPlot* hvaldisp_;

    void		saveHdr(CallBacker*);
    void		dispSeis(CallBacker*);
    void		dispHist(CallBacker*);
    void		updateInput(CallBacker*);
    void		vwrClose(CallBacker*);
    void		rowClck(CallBacker*);

    void		updateInp();
    void		setRow(int);
    void		handleFirstTrace(const SeisTrc&,
					 const SEGYSeisTrcTranslator&);
    bool		rejectOK(CallBacker*);

    void		outInfo(const uiString&);

    int			firsttrace_		= 1;
    void		firstTrcCB(CallBacker*);
    void		backCB(CallBacker*);
    void		forwardCB(CallBacker*);
    void		nrTrcsCB(CallBacker*);
    void		updateMaxTrace();

public:

    uiString		sGetWinTitle();

};


#define sKeySettNrTrcExamine \
    IOPar::compKey("SEG-Y",uiSEGYExamine::Setup::sKeyNrTrcs)


