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
    bool			createAttribLog(const DBKey&);
    bool			createAttribLog(const BufferStringSet&);
    bool			createLogCube(const DBKey&);
    bool			create2DFromWells(DBKey& newseisid,
						  Pos::GeomID& newlinegid);
    void			doXPlot();

    void 			setDPSDispMgr(DataPointSetDisplayMgr* dispmgr )
				{ dpsdispmgr_ = dispmgr; }
    bool			createD2TModel(const DBKey&);

    Pos::GeomID			new2DFromWellGeomID() const;
    bool			getPrev2DFromWellCoords(TypeSet<Coord>&);

    static int			evPreview2DFromWells();
    static int			evShow2DFromWells();
    static int			evCleanPreview();

    bool			showAmplSpectrum(const DBKey&,
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
    void			previewWellto2DLine(CallBacker*);
    void			wellTo2DDlgClosed(CallBacker*);

private:

    void			cleanUp();

};

