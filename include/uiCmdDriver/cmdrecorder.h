#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          March 2009
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "callback.h"
#include "separstr.h"
#include "od_ostream.h"

#include "cmddriverbasics.h"

class uiMenu;
class StreamData;
class uiMainWin;
class uiAction;


namespace CmdDrive
{

#define mRecOutStrm \
    if ( !rec_.mustSkip() ) rec_.outputStrm()


mExpClass(uiCmdDriver) CmdRecEvent
{
public:
			CmdRecEvent()
			    : begin_(true), refnr_(0) , srcwin_(0)
			    , openqdlg_(false), stolen_(false)
			    , object_(0), similarobjs_(true), mnuitm_(0)
			    , dynamicpopup_(false), casedep_(false)
			    , nraccepts_(0)
			{}

    BufferString	idstr_;
    bool		begin_;
    int			refnr_;
    BufferString	msg_;

    const uiMainWin*	srcwin_;
    bool		openqdlg_;
    BufferString	qdlgtitle_;
    bool		stolen_;

    CallBacker*		object_;
    bool		similarobjs_;
    BufferString	keystr_;
    uiAction*		mnuitm_;
    bool		dynamicpopup_;
    BufferString	menupath_;
    bool		casedep_;
    BufferString	execprefix_;

    int			nraccepts_;
};


class CmdComposer;

mExpClass(uiCmdDriver) CmdRecorder : public CallBacker
{
public:
    friend class	CmdComposer;

    			CmdRecorder(const uiMainWin& applwin);
    			~CmdRecorder();

    void		setOutputFile(const char* fnm)	{ outputfnm_ = fnm; }
    void		setBufferSize(int sz)		{ bufsize_ = sz; }
    void		writeTailOnly(bool yn)		{ writetailonly_ = yn; }
    void		ignoreCmdDriverWindows(bool);

    od_ostream& 	outputStrm() const;
    bool		mustSkip() const;

    bool		start();
    void		stop(bool fatal=false);

    bool 		isRecording() const		{ return recording_; }

    void		updateCmdComposers();

protected:

    void		handleEvent(CallBacker*);
    void                dynamicMenuInterceptor(CallBacker*);

    void		insertWinAssertion(const CmdRecEvent&);
    bool		findKeyString(const uiMainWin&,CmdRecEvent&);

    void		flush();

    const uiMainWin*	applWin() const			{ return applwin_; }
    const uiMainWin*	applwin_;

    CmdRecorder&	rec_;
    bool		recording_;
    BufferString	outputfnm_;
    od_ostream& 	outputstrm_;
    uiMenu*	        dynamicpopupmenu_;
    BufferString	winassertion_;
    bool		winassertcasedep_;

    WindowStack		winstack_;
    TypeSet<int>	popuprefnrs_;

    bool		openqdialog_;
    bool		ignorecmddriverwindows_;

    const CallBacker*		lastobjsearched_;
    ObjectSet<const uiMainWin>	lastobjfreewins_;	

    ObjectSet<CmdComposer>	composers_;

    bool		writetailonly_;
    od_ostrstream&	bufstream_;
    BufferString	bufstr_;
    int			bufsize_;
    int			nrparskipped_;
    mutable int		outputcounter_;
};


}; // namespace CmdDrive


