#ifndef uiwellattribpartserv_h
#define uiwellattribpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.h,v 1.16 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class DataPointSet;
class DataPointSetDisplayMgr;
class MultiID;
class NLAModel;
class uiWellAttribCrossPlot;

namespace Attrib { class DescSet; }

/*! \brief Part Server for Wells */

mClass uiWellAttribPartServer : public uiApplPartServer
{
public:
				uiWellAttribPartServer(uiApplService&);
				~uiWellAttribPartServer();

    void			setAttribSet(const Attrib::DescSet&);
    void			setNLAModel(const NLAModel*);
    const NLAModel*		getNLAModel()		{ return nlamodel;}
    void			importSEGYVSP();

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			createAttribLog(const MultiID&,int);
    void			doXPlot();

    void 			setDPSDispMgr(DataPointSetDisplayMgr* dispmgr )
				{ dpsdispmgr_ = dispmgr; }
    bool                        createD2TModel(const MultiID&);

protected:

    Attrib::DescSet*		attrset;
    const NLAModel*		nlamodel;

    void 			showSelPts(CallBacker*);
    void 			removeSelPts(CallBacker*);

    uiWellAttribCrossPlot*	xplotwin2d_;
    uiWellAttribCrossPlot*	xplotwin3d_;
    DataPointSetDisplayMgr*	dpsdispmgr_;
    int				dpsid_;
};

/*!\mainpage WellAttrib User Interface

  Here you will find all attribute handling regarding wells.
  The uiAttribWellPartServer delivers the services needed.

*/


#endif
