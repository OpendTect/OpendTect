#ifndef uiwellattribpartserv_h
#define uiwellattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.h,v 1.1 2004-03-01 14:29:50 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class MultiID;
class AttribDescSet;
class NLAModel;

/*! \brief Part Server for Wells */

class uiWellAttribPartServer : public uiApplPartServer
{
public:
				uiWellAttribPartServer(uiApplService&);
				~uiWellAttribPartServer();

    void			setAttribSet(const AttribDescSet&);
    void			setNLAModel(const NLAModel*);

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			selectAttribute(const MultiID&);

protected:

    AttribDescSet*		attrset;
    const NLAModel*		nlamodel;

};

/*!\mainpage AttribWell User Interface

  Here you will find all attributehandling regarding wells.
  The uiAttribWellPartServer delivers the services needed.

*/


#endif
