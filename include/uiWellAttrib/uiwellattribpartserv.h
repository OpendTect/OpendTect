#ifndef uiwellattribpartserv_h
#define uiwellattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.h,v 1.14 2009-04-27 11:54:57 cvssatyaki Exp $
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

    static const int		evGetDPSDispMgr();

    void			setAttribSet(const Attrib::DescSet&);
    void			setNLAModel(const NLAModel*);
    const NLAModel*		getNLAModel()		{ return nlamodel;}
    void			importSEGYVSP();

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			createAttribLog(const MultiID&,int);
    void			doXPlot();

    void			setVisDpsId( int id )	{ visdpsid_ = id; }
    int				visDpsId() const	{ return visdpsid_; }
    const DataPointSet&		getPointSet() const	{ return *dps_; }
    void 			setDPSDispMgr( DataPointSetDisplayMgr* dispmgr )
				{ dpsdispmgr_ = dispmgr; }
    bool                        createD2TModel(const MultiID&);

protected:

    Attrib::DescSet*		attrset;
    const NLAModel*		nlamodel;

    void 			showSelPts(CallBacker*);
    void 			removeSelPts(CallBacker*);

    int				visdpsid_;
    uiWellAttribCrossPlot*	xplotwin2d_;
    uiWellAttribCrossPlot*	xplotwin3d_;
    const DataPointSet*		dps_;
    DataPointSetDisplayMgr*	dpsdispmgr_;
};

/*!\mainpage WellAttrib User Interface

  Here you will find all attribute handling regarding wells.
  The uiAttribWellPartServer delivers the services needed.

*/


#endif
