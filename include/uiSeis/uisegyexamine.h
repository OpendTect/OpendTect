#ifndef uisegyexamine_h
#define uisegyexamine_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id: uisegyexamine.h,v 1.10 2011-03-08 15:02:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "segyfiledef.h"
class Timer;
class SeisTrc;
class uiTable;
class uiTextEdit;
class SeisTrcBuf;
class SeisTrcReader;
class uiFunctionDisplay;
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
    Timer&		timer_;

    uiTextEdit*		txtfld_;
    uiTable*		tbl_;
    uiFunctionDisplay*	funcdisp_;

    void		onStartUp(CallBacker*);
    void		saveHdr(CallBacker*);
    void		dispSeis(CallBacker*);
    void		updateInput(CallBacker*);
    void		vwrClose(CallBacker*);
    void		rowClck(CallBacker*);
    void		cellClck(CallBacker*);

    void		updateInp();
    void		setRow(int);
    void		handleFirstTrace(const SeisTrc&,
	    				 const SEGYSeisTrcTranslator&);
    bool		rejectOK();

    void		outInfo(const char*);

};


#endif
