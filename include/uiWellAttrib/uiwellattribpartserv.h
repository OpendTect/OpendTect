#ifndef uiwellattribpartserv_h
#define uiwellattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id: uiwellattribpartserv.h,v 1.11 2009-02-20 11:34:18 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class MultiID;
class NLAModel;
class uiWellAttribCrossPlot;
class uiD2TModelGenWin;

namespace Attrib { class DescSet; }
namespace Pick { class Set; }

/*! \brief Part Server for Wells */

mClass uiWellAttribPartServer : public uiApplPartServer
{
public:
				uiWellAttribPartServer(uiApplService&);
				~uiWellAttribPartServer();

    static const int		evShowPickSet();

    void			setAttribSet(const Attrib::DescSet&);
    void			setNLAModel(const NLAModel*);
    const NLAModel*		getNLAModel()		{ return nlamodel;}
    void			importSEGYVSP();

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			createAttribLog(const MultiID&,int);
    void			doXPlot();

    const Pick::Set&		getSelPickSet() const	{ return *selpickset_; }
    bool                        createD2TModel(const MultiID&);

protected:

    Attrib::DescSet*		attrset;
    const NLAModel*		nlamodel;

    void 			showPickSet(CallBacker*);
    uiWellAttribCrossPlot*	xplotwin2d_;
    uiWellAttribCrossPlot*	xplotwin3d_;
    uiD2TModelGenWin*           uid2tmgenwin_;
    const Pick::Set*		selpickset_;
};

/*!\mainpage WellAttrib User Interface

  Here you will find all attribute handling regarding wells.
  The uiAttribWellPartServer delivers the services needed.

*/


#endif
