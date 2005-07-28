#ifndef uiwellattribpartserv_h
#define uiwellattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.h,v 1.2 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class MultiID;
class NLAModel;

namespace Attrib { class DescSet; }

/*! \brief Part Server for Wells */

class uiWellAttribPartServer : public uiApplPartServer
{
public:
				uiWellAttribPartServer(uiApplService&);
				~uiWellAttribPartServer();

    void			setAttribSet(const Attrib::DescSet&);
    void			setNLAModel(const NLAModel*);

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			selectAttribute(const MultiID&);

protected:

    Attrib::DescSet*		attrset;
    const NLAModel*		nlamodel;

};

/*!\mainpage AttribWell User Interface

  Here you will find all attributehandling regarding wells.
  The uiAttribWellPartServer delivers the services needed.

*/


#endif
