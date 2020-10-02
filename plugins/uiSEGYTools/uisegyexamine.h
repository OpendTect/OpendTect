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
#include "uistring.h"

class Timer;
class SeisTrc;
class uiTable;
class uiTextEdit;
class SeisTrcBuf;
class uiSEGYTrcHdrValPlot;
class SEGYSeisTrcTranslator;


/* The dialog for examining SEG-Y files */

mExpClass(uiSEGYTools) uiSEGYExamine : public uiDialog
{ mODTextTranslationClass(uiSEGYExamine);
public:

    mStruct(uiSEGYTools) Setup : public uiDialog::Setup
    {
				Setup(int nrtraces=-1);

	mDefSetupMemb(int,nrtrcs)
	mDefSetupMemb(SEGY::FileSpec,fs)
	mDefSetupMemb(FilePars,fp)

	void			usePar(const IOPar&);
	static const char*	sKeyNrTrcs;
	static int		getDefNrTrcs();
    };

			uiSEGYExamine(uiParent*,const Setup&);
			~uiSEGYExamine();

    int			getRev() const;		//!< -1 = err
    const uiString&	errMsg() const		{ return txtinfo_; }

    static SEGYSeisTrcTranslator* getReader(const Setup&,uiString& errmsg);
    static int		getRev(const SEGYSeisTrcTranslator*); // -1 = err
    static int		getRev(const Setup&,uiString& emsg);

    static bool		launch(const Setup&);

protected:

    Setup		setup_;
    SEGYSeisTrcTranslator* segytransl_;
    uiString		txtinfo_;
    BufferString	fname_;
    SeisTrcBuf&		tbuf_;
    Timer*		timer_;

    uiTextEdit*		txtfld_;
    uiTable*		tbl_;
    uiSEGYTrcHdrValPlot* hvaldisp_;

    void		saveHdr(CallBacker*);
    void		dispSeis(CallBacker*);
    void		dispHist(CallBacker*);
    void		updateInput(CallBacker*);
    void		vwrClose(CallBacker*);
    void		rowClck(CallBacker*);

    void		updateInp();
    void		setRow(int);
    void		handleFirstTrace(const SeisTrc&);
    bool		rejectOK();

    void		outInfo(const uiString);

public:

    uiString		sGetWinTitle();

};


#define sKeySettNrTrcExamine \
    IOPar::compKey("SEG-Y",uiSEGYExamine::Setup::sKeyNrTrcs)
