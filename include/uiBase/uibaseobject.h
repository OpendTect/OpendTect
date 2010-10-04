#ifndef uibaseobject_h
#define uibaseobject_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id: uibaseobject.h,v 1.5 2010-10-04 05:02:36 cvsranojay Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

class uiBody;

mClass uiBaseObject : public NamedObject
{
public:
				uiBaseObject(const char* nm, uiBody*);
    virtual			~uiBaseObject();

				// implementation: uiobj.cc
    void			finalise();
    bool			finalised() const;
    void			clear();

    inline const uiBody*	body() const		{ return body_; }
    inline uiBody*		body()			{ return body_; }

    static void			setCmdRecorder(const CallBack&);
    static void			unsetCmdRecorder();

    int	 /* refnr */		beginCmdRecEvent(const char* msg=0);
    void			endCmdRecEvent(int refnr,const char* msg=0);

    int	 /* refnr */		beginCmdRecEvent(od_uint64 id,
	    					 const char* msg=0);
    void			endCmdRecEvent(od_uint64 id,int refnr,
					       const char* msg=0);

    Notifier<uiBaseObject>	finaliseStart;
				//!< triggered when about to start finalising
    Notifier<uiBaseObject>	finaliseDone;
    				//!< triggered when finalising finished
    Notifier<uiBaseObject>	tobeDeleted;
				//!< triggered in destructor

protected:

    void			setBody( uiBody* b )		{ body_ = b; }

private:
    static CallBack*		cmdrecorder_;
    int				cmdrecrefnr_;
    uiBody*			body_;
};


#endif
