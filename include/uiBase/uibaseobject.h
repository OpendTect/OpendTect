#ifndef uibaseobject_h
#define uibaseobject_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2001
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "namedobj.h"
#include "rowcol.h"

class uiBody;
mFDQtclass(QWidget)

/*! The main class for all UI-objects in OpendTect. Normally,
    there is one widget per class, but there can be multiple. If
    so, each widget has a relative position related to the others,
    as returned from getWidgetOrigin and getWidgetSpan. */

mExpClass(uiBase) uiBaseObject : public NamedObject
{
public:
				uiBaseObject(const char* nm, uiBody* = 0);
    virtual			~uiBaseObject();

				// implementation: uiobj.cc
    void			finalise();
    bool			finalised() const;
    void			clear();

    virtual void		translateText()		{}
				/*!<Triggers translation of object and all
				    members to current language. */

    inline const uiBody*	body() const		{ return body_; }
    inline uiBody*		body()			{ return body_; }

    static void			addCmdRecorder(const CallBack&);
    static void			removeCmdRecorder(const CallBack&);

    int	 /* refnr */		beginCmdRecEvent(const char* msg=0);
    void			endCmdRecEvent(int refnr,const char* msg=0);

    int	 /* refnr */		beginCmdRecEvent(od_uint64 id,
	    					 const char* msg=0);
    void			endCmdRecEvent(od_uint64 id,int refnr,
					       const char* msg=0);

    Notifier<uiBaseObject>	tobeDeleted;
				//!< triggered in destructor

    virtual Notifier<uiBaseObject>& preFinalise()
				{ return finaliseStart_; }
    virtual Notifier<uiBaseObject>& postFinalise()
				{ return finaliseDone_; }
    
    virtual int			getNrWidgets() const			= 0;
    virtual mQtclass(QWidget)*	getWidget(int widgetindex);
    const mQtclass(QWidget)*	getConstWidget(int widgetindex) const;

    virtual RowCol		getWidgetOrigin(int widgetindex) const;
    virtual RowCol		getWidgetSpan(int widgetindex) const;

    int				getNrRows() const;
    int				getNrCols() const;

protected:

    void			setBody( uiBody* b )	{ body_ = b; }

    Notifier<uiBaseObject>	finaliseStart_;
				//!< triggered when about to start finalising
    Notifier<uiBaseObject>	finaliseDone_;
    				//!< triggered when finalising finished

private:
    int				getNrRowCols( bool row ) const;
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


#endif
