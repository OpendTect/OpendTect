#ifndef uiemattribpartserv_h
#define uiemattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiemattribpartserv.h,v 1.4 2008-06-23 06:37:57 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
#include "emposid.h"

namespace Attrib { class DescSet; }
class NLAModel;

/*! \brief Part Server for Attribute handling on EarthModel objects */

class uiEMAttribPartServer : public uiApplPartServer
{
public:
				uiEMAttribPartServer(uiApplService&);
				~uiEMAttribPartServer()	{}

    const char*			name() const		{ return "EMAttribs"; }

    enum HorOutType		{ OnHor, AroundHor, BetweenHors };
    void			createHorizonOutput(HorOutType);

    void			snapHorizon(const EM::ObjectID&);

    void			setNLA( const NLAModel* mdl, const MultiID& id )
				{ nlamodel_ = mdl; nlaid_ = id; }
    void			setDescSet( const Attrib::DescSet* ads )
				{ descset_ = ads; }

    void			import2DHorizon() const;

protected:

    const NLAModel*		nlamodel_;
    const Attrib::DescSet*	descset_;
    MultiID			nlaid_;

};

/*!\mainpage EMAttrib User Interface

  Here you will find all attribute handling regarding EarthModel objects.
  The uiEMAttribPartServer delivers the services needed.

*/


#endif
