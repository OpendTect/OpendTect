#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "namedobj.h"

class uiBody;
mFDQtclass(QWidget)

mExpClass(uiBase) uiBaseObject : public NamedCallBacker
{
public:
    virtual			~uiBaseObject();
				mOD_DisableCopy(uiBaseObject)

				// implementation: uiobj.cc
    void			finalize();
    bool			finalized() const;
    void			clear();

    virtual void		translateText()		{}
				/*!<Triggers translation of object and all
				    members to current language. */

    inline const uiBody*	body() const		{ return body_; }
    inline uiBody*		body()			{ return body_; }

    static void			addCmdRecorder(const CallBack&);
    static void			removeCmdRecorder(const CallBack&);

    int	 /* refnr */		beginCmdRecEvent(const char* msg=0);
    int  /* refnr */		beginCmdRecEvent(const BufferString& msg);
    void			endCmdRecEvent(int refnr,const char* msg=0);

    int	 /* refnr */		beginCmdRecEvent(od_uint64 id,
						 const char* msg=0);
    void			endCmdRecEvent(od_uint64 id,int refnr,
					       const char* msg=0);

    virtual Notifier<uiBaseObject>& preFinalize()
				{ return finalizeStart; }
    virtual Notifier<uiBaseObject>& postFinalize()
				{ return finalizeDone; }

    virtual mQtclass(QWidget*)	getWidget() { return 0; }
    const mQtclass(QWidget*)	getWidget() const;


    mDeprecated("Use preFinalize()")
    virtual Notifier<uiBaseObject>& preFinalise()
				{ return preFinalize(); }
    mDeprecated("Use postFinalize()")
    virtual Notifier<uiBaseObject>& postFinalise()
				{ return postFinalize(); }
    mDeprecated("Use finalize()")
    void			finalise()		{ finalize(); }
    mDeprecated("Use finalized()")
    bool			finalised() const	{ return finalized(); }

protected:
				uiBaseObject(const char* nm,uiBody* =nullptr);

    void			setBody( uiBody* b )	{ body_ = b; }

private:
    Notifier<uiBaseObject>	finalizeStart;
				//!< triggered when about to start finalizing
    Notifier<uiBaseObject>	finalizeDone;
				//!< triggered when finalizing finished

private:
    int				cmdrecrefnr_;
    uiBody*			body_;
};


/*
CmdRecorder annotation to distinguish real user actions from actions
performed by program code. Should be used at start of each (non-const)
uiObject function that calls any uiBody/Qt function that may trigger a
signal received by the corresponding Messenger class (see i_q****.h).
Apart from a few notify handler functions, it will do no harm when
using this annotation unnecessarily.
*/

#define mBlockCmdRec		CmdRecStopper cmdrecstopper(this);

mExpClass(uiBase) CmdRecStopper
{
public:
				CmdRecStopper(const uiBaseObject*);
				~CmdRecStopper();

    static void			clearStopperList(const CallBacker* cmdrec);
				//!< will clear after all cmdrecs have called

    static bool			isInStopperList(const uiBaseObject* obj);
};
