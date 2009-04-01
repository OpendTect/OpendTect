#ifndef uiwellattribpartserv_h
#define uiwellattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.h,v 1.12 2009-04-01 07:38:39 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class DataPointSet;
class MultiID;
class NLAModel;
class uiWellAttribCrossPlot;
class uiD2TModelGenWin;

namespace Attrib { class DescSet; }

/*! \brief Part Server for Wells */

mClass uiWellAttribPartServer : public uiApplPartServer
{
public:
				uiWellAttribPartServer(uiApplService&);
				~uiWellAttribPartServer();

    static const int		evShowSelPoints();
    static const int		evRemoveSelPoints();

    void			setAttribSet(const Attrib::DescSet&);
    void			setNLAModel(const NLAModel*);
    const NLAModel*		getNLAModel()		{ return nlamodel;}
    void			importSEGYVSP();

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			createAttribLog(const MultiID&,int);
    void			doXPlot();

    const DataPointSet&		getPointSet() const	{ return *dps_; }
    bool                        createD2TModel(const MultiID&);

protected:

    Attrib::DescSet*		attrset;
    const NLAModel*		nlamodel;

    void 			showSelPts(CallBacker*);
    void 			removeSelPts(CallBacker*);

    uiWellAttribCrossPlot*	xplotwin2d_;
    uiWellAttribCrossPlot*	xplotwin3d_;
    uiD2TModelGenWin*           uid2tmgenwin_;
    const DataPointSet*		dps_;
};

/*!\mainpage WellAttrib User Interface

  Here you will find all attribute handling regarding wells.
  The uiAttribWellPartServer delivers the services needed.

*/


#endif
