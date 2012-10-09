#ifndef uisegyexamine_h
#define uisegyexamine_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "segyfiledef.h"
class Timer;
class SeisTrc;
class uiLabel;
class uiTable;
class uiTextEdit;
class uiGenInput;
class SeisTrcBuf;
class SeisTrcReader;
class uiSEGYTrcHdrValPlot;
class SEGYSeisTrcTranslator;


mClass uiSEGYExamine : public uiDialog
{
public:

    mStruct Setup : public uiDialog::Setup
    {
				Setup(int nrtraces=100);

	mDefSetupMemb(int,nrtrcs)
	mDefSetupMemb(SEGY::FileSpec,fs)
	mDefSetupMemb(SEGY::FilePars,fp)

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
    uiSEGYTrcHdrValPlot* hvaldisp_;

    void		onStartUp(CallBacker*);
    void		saveHdr(CallBacker*);
    void		dispSeis(CallBacker*);
    void		updateInput(CallBacker*);
    void		vwrClose(CallBacker*);
    void		rowClck(CallBacker*);

    void		updateInp();
    void		setRow(int);
    void		handleFirstTrace(const SeisTrc&,
	    				 const SEGYSeisTrcTranslator&);
    bool		rejectOK(CallBacker*);

    void		outInfo(const char*);

};


#endif
