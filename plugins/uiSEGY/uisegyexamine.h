#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
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
namespace Seis { class Provider; }


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

    const uiString&	errMsg() const		{ return txtinfo_; }

    static Seis::Provider* getProvider(const Setup&,uiString& errmsg);
    static int		getRev(const uiSEGYExamine::Setup&);
			//!< -1 = err, 1 = Rev 1
    static int		getRev(const Setup&,uiString& emsg);
    static bool		launch(const Setup&);

protected:

    Setup		setup_;
    Seis::Provider*	prov_;
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
    void		handleFirstTrace(const SeisTrc&,
					 const SEGYSeisTrcTranslator&);
    bool		rejectOK();

    void		outInfo(const uiString);

public:
    uiString		sGetWinTitle();

};


#define sKeySettNrTrcExamine \
    IOPar::compKey("SEG-Y",uiSEGYExamine::Setup::sKeyNrTrcs)
