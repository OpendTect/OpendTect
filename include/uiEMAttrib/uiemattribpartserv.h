#ifndef uiemattribpartserv_h
#define uiemattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiemattribpartserv.h,v 1.1 2006-09-19 09:28:11 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

/*! \brief Part Server for Attribute handling on EarthModel objects */

class uiEMAttribPartServer : public uiApplPartServer
{
public:
				uiEMAttribPartServer(uiApplService&);
				~uiEMAttribPartServer();

    const char*			name() const		{ return "EMAttribs"; }

    void			snapHorizon();

protected:

};

/*!\mainpage EMAttrib User Interface

  Here you will find all attribute handling regarding EarthModel objects.
  The uiEMAttribPartServer delivers the services needed.

*/


#endif
