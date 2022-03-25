#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2004
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uiapplserv.h"
#include "uistring.h"

class BufferStringSet;
class DataPointSet;
class DataPointSetDisplayMgr;
class NLAModel;
class uiCreateLogCubeDlg;
class uiWellAttribCrossPlot;
class uiWellTo2DLineDlg;

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
    bool			create2DFromWells(MultiID& newseisid,
						  Pos::GeomID& newlinegid);
    void			doXPlot();

    void 			setDPSDispMgr(DataPointSetDisplayMgr* dispmgr )
				{ dpsdispmgr_ = dispmgr; }
    bool			createD2TModel(const MultiID&);

    Pos::GeomID			new2DFromWellGeomID() const;
    bool			getPrev2DFromWellCoords(TypeSet<Coord>&);

    static int			evPreview2DFromWells();
    static int			evShow2DFromWells();
    static int			evCleanPreview();

    bool			showAmplSpectrum(const MultiID&,
						 const char* lognm);

protected:

    Attrib::DescSet*		attrset_;
    const NLAModel*		nlamodel_;

    WellTie::uiTieWinMGRDlg*	welltiedlg_;
    uiWellAttribCrossPlot*	xplotwin2d_;
    uiWellAttribCrossPlot*	xplotwin3d_;
    uiWellTo2DLineDlg*		wellto2ddlg_;
    uiCreateLogCubeDlg*		crlogcubedlg_;
    DataPointSetDisplayMgr*	dpsdispmgr_;

    void			surveyChangedCB(CallBacker*);
    void			wellManCreatedCB(CallBacker*);
    void			xplotCB(CallBacker*);
    void			previewWellto2DLine(CallBacker*);
    void			wellTo2DDlgClosed(CallBacker*);
    void			welltieDlgClosedCB(CallBacker*);

private:

    void			cleanUp();

};

