#ifndef uiwellattribpartserv_h
#define uiwellattribpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uiapplserv.h"
#include "uistring.h"

class BufferStringSet;
class DataPointSet;
class DataPointSetDisplayMgr;
class NLAModel;
class uiSEGYRead;
class uiWellAttribCrossPlot;

namespace Attrib { class DescSet; }
namespace WellTie { class uiTieWinMGRDlg; }

/*!
\ingroup uiWellAttrib
\brief Part Server for Wells
*/

mExpClass(uiWellAttrib) uiWellAttribPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiWellAttribPartServer);
public:
				uiWellAttribPartServer(uiApplService&);
				~uiWellAttribPartServer();

    void			setAttribSet(const Attrib::DescSet&);
    void			setNLAModel(const NLAModel*);
    const NLAModel*		getNLAModel()		{ return nlamodel_;}

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			createAttribLog(const MultiID&);
    bool			createAttribLog(const BufferStringSet&);
    bool			createLogCube(const MultiID&);
    void			doXPlot();
    void			doSEGYTool(IOPar* prevpars=0,int choice=-1);
    void			doVSPTool(IOPar* prevpars=0,int choice=-1);

    void 			setDPSDispMgr(DataPointSetDisplayMgr* dispmgr )
				{ dpsdispmgr_ = dispmgr; }
    bool			createD2TModel(const MultiID&);

    bool			showAmplSpectrum(const MultiID&,
						 const char* lognm);

protected:

    Attrib::DescSet*		attrset_;
    const NLAModel*		nlamodel_;
    bool			welltiedlgopened_;
    uiSEGYRead*			cursegyread_;

    WellTie::uiTieWinMGRDlg*	welltiedlg_;
    uiWellAttribCrossPlot*	xplotwin2d_;
    uiWellAttribCrossPlot*	xplotwin3d_;
    DataPointSetDisplayMgr*	dpsdispmgr_;

    void                        closeWellTieDlg(CallBacker*);
    void                        surveyChangedCB(CallBacker*);
    void                        segyToolEnded(CallBacker*);
    bool			launchSEGYWiz(IOPar*,int);

private:
    void			cleanUp();
};

#endif
