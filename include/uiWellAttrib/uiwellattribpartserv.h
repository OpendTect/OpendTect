#ifndef uiwellattribpartserv_h
#define uiwellattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.h,v 1.15 2009-05-21 09:05:10 cvssatyaki Exp $
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
