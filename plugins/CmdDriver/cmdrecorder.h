#ifndef cmdrecorder_h
#define cmdrecorder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          March 2009
 RCS:           $Id: cmdrecorder.h,v 1.20 2010/04/15 15:44:19 cvsjaap Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "separstr.h"

#include "cmddriverbasics.h"
 
class uiPopupMenu;
class BufferString;
class StreamData;
class uiMainWin;
class uiObject;
class uiMenuItem;


namespace CmdDrive
{

#define mRecOutStrm \
    if ( rec_.logStrmData().usable() ) *rec_.logStrmData().ostrm


mClass CmdRecEvent
{
public:
			CmdRecEvent()
			    : begin_(true), refnr_(0) , srcwin_(0)
			    , openqdlg_(false), stolen_(false)
			    , object_(0), similarobjs_(true), mnuitm_(0)
			    , dynamicpopup_(false), casedep_(false)
			    , nraccepts_(0), interrupt_(false)
			{}

    BufferString	idstr_;
    bool		begin_;
    int			refnr_;
    BufferString	msg_;

    const uiMainWin*	srcwin_;
    bool		openqdlg_;
    BufferString	qdlgtitle_;
    bool		stolen_;

    uiObject*		object_;
    bool		similarobjs_;
    BufferString	keystr_;
    const uiMenuItem*	mnuitm_;
    bool		dynamicpopup_;
    BufferString	menupath_;
    bool		casedep_;
    BufferString	execprefix_;

    int			nraccepts_;
    bool		interrupt_;

};


class CmdComposer;

mClass CmdRecorder : public CallBacker
{
public:
    friend class	CmdComposer;

    			CmdRecorder();
    			~CmdRecorder();

    void		setOutputFile(const char* fnm)	{ outputfnm_= fnm; }
    const StreamData&   logStrmData() const		{ return outputsd_; }

    bool		start();
    void		stop();
    bool 		isRecording() const		{ return recording_; }

protected:

    void		handleEvent(CallBacker*);
    void                dynamicMenuInterceptor(CallBacker*);

    void		insertWinAssertion(const CmdRecEvent&);
    bool		findKeyString(const uiMainWin&,CmdRecEvent&);

    CmdRecorder&	rec_;
    bool		recording_;
    BufferString	outputfnm_;
    StreamData&		outputsd_;
    uiPopupMenu*        dynamicpopupmenu_;
    const uiMainWin*	currootwin_;
    BufferString	winassertion_;
    bool		winassertcasedep_;

    WindowStack		winstack_;
    TypeSet<int>	popuprefnrs_;

    bool		openqdialog_;

    const uiObject*		lastobjsearched_;
    ObjectSet<const uiMainWin>	lastobjfreewins_;	

    ObjectSet<CmdComposer>	composers_;
};


}; // namespace CmdDrive


#endif
