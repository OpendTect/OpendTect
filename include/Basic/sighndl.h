#ifndef sighndl_h
#define sighndl_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: sighndl.h,v 1.1 2001-04-27 16:48:45 bert Exp $
________________________________________________________________________

-*/

#include <callback.h>

/*!\brief asynchronous event handling and notification. */


class SignalHandling : public CallBacker
{
public:

    enum EvType			{
				    ConnClose,
				    ChldStop,
				    ReInit,
				    Stop,
				    Cont,
				    Kill	// This process
				};

    static void			startNotify(EvType,const CallBack&);
    static void			stopNotify(EvType,const CallBack&);

protected:

				SignalHandling();
    static SignalHandling	theinst_;

    CallBackList		conncbs;
    CallBackList		chldcbs;
    CallBackList		reinitcbs;
    CallBackList		stopcbs;
    CallBackList		contcbs;
    CallBackList		killcbs;

    CallBackList&		getCBL(EvType);

#ifndef __msvc__

    static void			handle(int);

    void			doKill(int);
    void			doStop(int);
    void			doCont();
    void			handleConn();
    void			handleChld();
    void			handleReInit();
#endif

};


#endif
