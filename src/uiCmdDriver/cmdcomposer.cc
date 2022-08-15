/*+

________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		March 2009
________________________________________________________________________

-*/

#include "cmdcomposer.h"

#include "cmddriverbasics.h"
#include "cmdrecorder.h"

#include "canvascommands.h"
#include "drivercommands.h"
#include "inputcommands.h"
#include "listcommands.h"
#include "menubutcommands.h"
#include "qtcommands.h"
#include "tablecommands.h"
#include "treecommands.h"

#include "timer.h"
#include "uimsg.h"


namespace CmdDrive
{

mImplFactory1Param( CmdComposer, CmdRecorder&, CmdComposer::factory );

static ObjectSet<const Classifier> classifiers;


#define mComposeFactoryKey( fackey, basekey, extrakey ) \
{ \
    fackey = basekey; \
    if ( fackey=="uiMainWin" && extrakey ) \
    { \
	fackey += "_"; fackey += extrakey; \
    } \
    StringProcessor(fackey).capitalize(); \
}

BufferString CmdComposer::factoryKey( const CallBacker* caller,
				      const char* extrakey )
{
    BufferString fackey;
    for ( int idx=0; idx<classifiers.size(); idx++ )
    {
	if ( classifiers[idx]->approved(caller) )
	{
	    mComposeFactoryKey( fackey, classifiers[idx]->name(), extrakey );
	    break;
	}
    }

    return fackey;
}


BufferString CmdComposer::createFactoryKey( const Classifier* classifier,
					    const char* keyword )
{
    StringView classifiername = classifier->name();
    classifiers.insertAt( classifier, 0 );
    for ( int idx=classifiers.size()-1; idx>0; idx-- )
    {
	if ( classifiername == classifiers[idx]->name() )
	    delete classifiers.removeSingle( idx );
    }

    BufferString fackey;
    mComposeFactoryKey( fackey, classifier->name(), keyword );

    if ( factory().hasName(fackey) )
    {
	BufferString errmsg( "Redefining composer \"" );
	errmsg += keyword; errmsg += "\"";
	pFreeFnErrMsg( errmsg );
    }

    return fackey;
}


CmdComposer::CmdComposer( CmdRecorder& cmdrec )
    : rec_( cmdrec )
    , done_( false )
    , ignoreflag_( false )
    , quitflag_( false )
    , updateflag_( false )
    , objclosed_( false )
{
    init();
}


CmdComposer::~CmdComposer()
{
    detachAllNotifiers();
    deepErase( eventlist_ );
}


void CmdComposer::srcWinClosed( CallBacker* )
{
    accept( CmdRecEvent() );	// Dummy event to flush pending command(s)
}


void CmdComposer::initStandardComposers()
{
    mDefineStaticLocalObject( bool, done, = false );
    if ( done ) return;
    done = true;

    MenuCmdComposer::initClass();
    ButtonCmdComposer::initClass();
    InputCmdComposer::initClass();
    SpinCmdComposer::initClass();
    SliderCmdComposer::initClass();
    ComboCmdComposer::initClass();
    ListCmdComposer::initClass();
    TabCmdComposer::initClass();
    TableCmdComposer::initClass();
    TreeCmdComposer::initClass();
    CanvasMenuCmdComposer::initClass();
    MdiAreaCmdComposer::initClass();
    CloseCmdComposer::initClass();
    QMsgBoxButCmdComposer::initClass();
    QColorDlgCmdComposer::initClass();
    QFileDlgCmdComposer::initClass();
}


const uiMainWin* CmdComposer::applWin() const
{ return rec_.applWin(); }


bool CmdComposer::greedy() const
{ return !refnrstack_.isEmpty(); }


bool CmdComposer::accept( const CmdRecEvent& ev )
{
    if ( objclosed_ )
    {
	stackwasempty_ = true;
	mRefuseAndQuit();
    }

    stackwasempty_ = refnrstack_.isEmpty();
    ignoreflag_ = false;

    if ( eventlist_.isEmpty() || eventlist_[0]->idstr_==ev.idstr_ )
    {
	const bool isvoidevent = eventNameIdx(voideventnames_,ev) >= 0;
	if ( isvoidevent )
	{
	    ignoreflag_ = true;
	    if ( eventlist_.isEmpty() )
		return false;
	}
	else
	    addToEventList( ev );

	if ( !refnrstack_.isEmpty() && ev.refnr_==refnrstack_[0] )
	{
	    refnrstack_.removeSingle( 0 );

	    if ( ev.begin_ )
		// in case accept(ev) was called tail-recursively
		stackwasempty_ = refnrstack_.isEmpty();
	    else
	    {
		done_ = !isvoidevent && refnrstack_.isEmpty();
		return true;
	    }
	}

	if ( ev.begin_ && (refnrstack_.isEmpty() || ev.stolen_) )
	{
	    refnrstack_.insert( 0, ev.refnr_ );
	    return true;
	}

	ignoreflag_ = true;
	return true;
    }

    if ( !refnrstack_.isEmpty() && !ev.stolen_ && !ev.dynamicpopup_ &&
	 eventlist_[0]->srcwin_==ev.srcwin_ &&
	 eventlist_[0]->openqdlg_==ev.openqdlg_ )
    {
	ignoreflag_ = true;
	return true;
    }

    return false;
}


bool CmdComposer::traceSrcWin( CmdRecEvent& ev ) const
{
    StringView evidstr( ev.idstr_ );
    for ( int idx=0; idx<eventlist_.size(); idx++ )
    {
	if ( eventlist_[idx]->idstr_==evidstr )
	{
	    ev.object_ = eventlist_[idx]->object_;
	    ev.mnuitm_ = eventlist_[idx]->mnuitm_;
	    ev.srcwin_ = eventlist_[idx]->srcwin_;

	    ev.openqdlg_ = eventlist_[idx]->openqdlg_;
	    ev.qdlgtitle_ = eventlist_[idx]->qdlgtitle_;

	    ev.keystr_ = eventlist_[idx]->keystr_;
	    ev.casedep_ = eventlist_[idx]->casedep_;
	    ev.similarobjs_ = eventlist_[idx]->similarobjs_;
	    ev.menupath_ = eventlist_[idx]->menupath_;
	    ev.execprefix_ = eventlist_[idx]->execprefix_;

	    return true;
	}
    }
    return false;
}


void CmdComposer::addToEventList( const CmdRecEvent& ev )
{
    mDynamicCastGet( uiObject*, uiobj, ev.object_ );
    if ( eventlist_.isEmpty() && uiobj )
	mAttachCB( uiobj->closed, CmdComposer::objClosed );

    uiMainWin* uimw = const_cast<uiMainWin*>( ev.srcwin_ );
    if ( eventlist_.isEmpty() && uimw )
	mAttachCB( uimw->windowClosed, CmdComposer::srcWinClosed );

    eventlist_ += new CmdRecEvent( ev );

    const int sz = eventlist_.size();
    const int burstevnameidx = eventNameIdx( bursteventnames_, ev );
    if ( !ev.begin_ || burstevnameidx<0 || sz<5 )
	return;

    for ( int idx=sz-2; idx>=sz-5; idx-- )
    {
	bool isbegin = ((sz-idx)%2)==1;
	if ( eventlist_[idx]->begin_ != isbegin )
	    return;
	if ( burstevnameidx != eventNameIdx(bursteventnames_,*eventlist_[idx]) )
	    return;
    }

    delete eventlist_.removeSingle( sz-2 );
    delete eventlist_.removeSingle( sz-3 );
}



void CmdComposer::shrinkEventList( int firstnr, int lastnr )
{
    const int sz = eventlist_.size();
    const int firstidx = firstnr<0 ? sz+firstnr : firstnr-1;
    const int lastidx  = lastnr<0  ? sz+lastnr  : lastnr-1;

    if ( firstidx<0 || lastidx>=sz || firstidx>lastidx )
	return;

    for ( int idx=sz-1; idx>=0; idx-- )
    {
	if ( idx>=firstidx && idx<=lastidx )
	    delete eventlist_.removeSingle( idx );
    }
}


int CmdComposer::eventNameIdx( const BufferStringSet& eventnames,
			       const CmdRecEvent& ev ) const
{
    const char* msgptr = ev.msg_;
    while ( msgptr && *msgptr )
    {
	BufferString nextword;
	msgptr = getNextNonBlanks( msgptr, nextword.getCStr() );
	const int idx = eventnames.indexOf(nextword);
	if ( idx >= 0 )
	    return idx;
    }
    return -1;
}


void CmdComposer::insertWindowCaseExec( const CmdRecEvent& event,
					bool casedep ) const
{
    rec_.insertWinAssertion( event );

    if ( Timer::setUserWaitFlag(false) )
    {
	mRecOutStrm << "Pause \"The user may determine the waiting time at "
	    << "this point. Click 'Resume' or replace the current script line "
	    << "by an isochronous Sleep-command.\"" << od_endl;
    }

    if ( event.casedep_ || casedep )
	mRecOutStrm << "Case Sensitive" << od_endl;

    mRecOutStrm << event.execprefix_;
}


bool CmdComposer::tryToFinish()
{
    for ( int count=refnrstack_.size()-1; count>=0; count-- )
    {
	for ( int idx=eventlist_.size()-1; idx>=0; idx-- )
	{
	    if ( eventlist_[idx]->refnr_==refnrstack_[0] )
	    {
		CmdRecEvent ev = *eventlist_[idx];
		if ( ev.openqdlg_ )
		    return false;

		ev.begin_ = false;
		accept( ev );
		break;
	    }
	}

	if ( count != refnrstack_.size() )
	    return false;
    }

    return true;
}


void CmdComposer::testCB(CallBacker*)
{ uiMSG().about(tr("CmdComposer test-callback handler")); }


} // namespace CmdDrive
