#ifndef uibaseobject_h
#define uibaseobject_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id: uibaseobject.h,v 1.1 2009-03-19 08:03:16 cvsnanne Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

class uiBody;

mClass uiBaseObject : public NamedObject
{
public:
				uiBaseObject( const char* nm, uiBody* b )
				    : NamedObject(nm)
				    , finaliseStart(this)
				    , finaliseDone(this)
				    , body_(b)			{}
    virtual			~uiBaseObject()			{}

				// implementation: uiobj.cc
    void			finalise();
    bool			finalised() const;
    void			clear();

    inline const uiBody*	body() const		{ return body_; }
    inline uiBody*		body()			{ return body_; }

    static void			setCmdRecorder(const CallBack&);
    static void			unsetCmdRecorder();
    void			markCmdRecEvent(bool start,const char* msg=0);

    Notifier<uiBaseObject>	finaliseStart;
				//!< triggered when about to start finalising
    Notifier<uiBaseObject>	finaliseDone;
    				//!< triggered when finalising finished

protected:

    void			setBody( uiBody* b )		{ body_ = b; }

private:
    static CallBack*		cmdrecorder_;
    uiBody*			body_;
};


#endif
