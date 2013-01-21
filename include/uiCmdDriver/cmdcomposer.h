#ifndef cmdcomposer_h
#define cmdcomposer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          March 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "callback.h"
#include "bufstringset.h"
#include "factory.h"

class uiMainWin;


namespace CmdDrive
{

class CmdRecorder;
class CmdRecEvent;


mExpClass(uiCmdDriver) Classifier
{
public:
    virtual		~Classifier()				{}
    virtual const char* name() const				= 0;
    virtual bool	approved(const CallBacker*) const	= 0;
};

#define mDeclClassifierClass(callerclass) \
\
class callerclass##Classifier : public Classifier \
{ \
public: \
\
    static const char*	className()	{ return #callerclass; } \
    const char*		name() const	{ return className(); } \
\
    bool		approved(const CallBacker* caller) const \
			{ return dynamic_cast<const callerclass*>(caller); } \
};


mExpClass(uiCmdDriver) CmdComposer : public CallBacker
{

public:

    mDefineFactory1ParamInClass( CmdComposer, CmdRecorder&, factory );
    static void		initStandardComposers();
    static BufferString	factoryKey(const CallBacker* caller,
	    			   const char* extrakey=0);

			CmdComposer(CmdRecorder&);
			~CmdComposer();

    virtual const char*	name()				= 0;

    virtual bool	greedy() const;
    virtual bool	accept(const CmdRecEvent&); 		
    virtual void	updateInternalState()		{};
    virtual bool	tryToFinish();
    bool		stateUpdateNeeded()		{ return updateflag_; }
    bool		done() const 			{ return done_; }

    bool		traceSrcWin(CmdRecEvent&) const;

    void		objClosed(CallBacker*)		{ objclosed_ = true; }
    void		testCB(CallBacker*); 

protected:

    static BufferString	createFactoryKey(const Classifier*,const char* keyword);

    virtual void	init()				{};

    const uiMainWin*	applWin() const;

    void		addToEventList(const CmdRecEvent&);
    void		shrinkEventList(int firstnr=1, int lastnr=-1);
    int			eventNameIdx(const BufferStringSet& eventnames,
				     const CmdRecEvent&) const;

    void		insertWindowCaseExec(const CmdRecEvent&,
					     bool casedep = false) const;

    void		notDone()			{ done_ = false; }
    void		refuseAndQuitDone()		{ done_ = true; }

    CmdRecorder&	rec_;

    bool		ignoreflag_;
    bool		quitflag_;
    bool		updateflag_;

    TypeSet<int>	refnrstack_;
    bool		stackwasempty_;
    bool		objclosed_;

    ObjectSet<CmdRecEvent> eventlist_;

    BufferStringSet	bursteventnames_;
    BufferStringSet	voideventnames_;

private:
    bool		done_;
};


#define mStartDeclComposerClassNoAccept(mod,cmdkey,parentclass) \
\
mExpClass(mod) cmdkey##CmdComposer : public parentclass \
{ \
public: \
			cmdkey##CmdComposer(CmdRecorder& cmdrec) \
			    : parentclass(cmdrec) \
    			{ init(); } \
\
    static const char*	keyWord()			{ return #cmdkey; } \
    virtual const char* name()				{ return keyWord(); } \

#define mStartDeclComposerClass(mod,cmdkey,parentclass,callerclass) \
\
    mStartDeclComposerClassNoAccept(mod,cmdkey,parentclass) \
    mDeclClassifierClass(callerclass) \
\
    virtual bool	accept(const CmdRecEvent&); \
\
    static CmdComposer*	createInstance(CmdRecorder& cmdrec) \
			{ return new cmdkey##CmdComposer(cmdrec); } \
    static void		initClass() \
    			{ factory().addCreator( createInstance, \
			    createFactoryKey(new callerclass##Classifier(), \
					     keyWord()) ); }

#define mStartDeclComposerClassWithInit(mod,cmdkey,parentclass,callerclass) \
    mStartDeclComposerClass(mod,cmdkey,parentclass,callerclass) \
    virtual void	init();

#define mEndDeclComposerClass \
};


//====== CmdComposer macros ==================================================


#define mRefuseAndQuit() \
{ \
    quitflag_ = true; \
    if ( stackwasempty_ ) \
	refuseAndQuitDone(); \
\
    return false; \
}


#define mNotifyTest( objclass, uiobject, notifiername ) \
{ \
    mDynamicCastGet( objclass*, uiclassobj, uiobject ); \
    if ( uiclassobj ) \
	uiclassobj->notifiername.notify( mCB(this,CmdComposer,testCB) ); \
}


#define mGetInputString( inpptr, txt, haschanged ) \
\
    BufferString inpstr; \
    char* inpptr = inpstr.buf(); \
    if ( haschanged ) \
    { \
	inpstr = " "; inpstr += txt; \
	mSkipBlanks( inpptr ); \
	char* endptr; \
	strtod( inpptr, &endptr ); \
	const char* nextword = endptr; \
	mSkipBlanks( nextword ); \
	if ( inpptr!=endptr && !*nextword ) \
	{ \
	    *endptr = '\0'; \
	    inpstr += " "; \
	} \
	else \
	{ \
	    mDressUserInputString( inpstr, sInputStr ); \
	    inpstr += "\" "; \
	    inpptr = inpstr.buf(); \
	    *inpptr = '"'; \
	} \
    }

#define mWriteInputCmd( haschanged, txt, enter ) \
{ \
    mGetInputString( inpptr, txt, haschanged ); \
    if ( haschanged || enter ) \
    { \
	insertWindowCaseExec( *eventlist_[0] ); \
	mRecOutStrm << "Input \"" << eventlist_[0]->keystr_ << "\" " \
		    << inpptr << (enter ? "Enter" : "Hold") << std::endl; \
    } \
} 


#define mGetItemName( uiobj,sizefunc,textfunc,curitemidx,curitemname,casedep ) \
\
    BufferString curitemname = uiobj->textfunc( curitemidx ); \
    mDressNameString( curitemname, sItemName ); \
    bool casedep = false; \
    { \
	int nrmatches = 0; \
	int selnr = 0; \
\
	for ( int itmidx=0; itmidx<uiobj->sizefunc(); itmidx++ ) \
	{ \
	    const char* itmtxt = uiobj->textfunc( itmidx ); \
	    if ( SearchKey(curitemname,false).isMatching(itmtxt) ) \
	    { \
		if ( SearchKey(curitemname,true).isMatching(itmtxt) ) \
		{ \
		    nrmatches++; \
		    if ( itmidx == curitemidx ) \
			selnr = nrmatches; \
		} \
		else \
		    casedep = true; \
	    } \
	} \
\
	if ( selnr && nrmatches>1 ) \
	{ \
	    curitemname += "#"; curitemname += selnr; \
	} \
    }


}; // namespace CmdDrive


#endif

