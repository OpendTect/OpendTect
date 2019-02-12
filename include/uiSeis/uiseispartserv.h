#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2002
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiapplserv.h"
#include "dbkey.h"
#include "geomid.h"
#include "uistring.h"

class BufferStringSet;
class TrcKeyZSampling;
class IOPar;
class SeisTrcBuf;
class uiBatchTime2DepthSetup;
class uiFlatViewWin;
class uiSeisFileMan;
class uiSeisImportODCube;
class uiSeisImpCubeFromOtherSurveyDlg;
class uiSeisExpCubePositionsDlg;
class uiSeisIOSimple;
class uiSeisPreStackMan;
class uiSeisWvltMan;
class uiSeisWvltImp;
class uiSeisWvltExp;

namespace PosInfo { class Line2DData; }
namespace Geometry { class RandomLine; }

/*!
\brief Seismic User Interface Part Server
*/

mExpClass(uiSeis) uiSeisPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiSeisPartServer);
public:
			uiSeisPartServer(uiApplService&);
			~uiSeisPartServer();

    const char*		name() const			{ return "Seismics"; }

    bool		importSeis(int opt);
    bool		exportSeis(int opt);

    DBKey		getDefaultDataID(bool is2d) const;
    bool		select2DSeis(DBKey&);
    bool		select2DLines(GeomIDSet&,int& action);
    static void		get2DStoredAttribs(const char* linenm,
				       BufferStringSet& attribs,int steerpol=2);
    void		get2DZdomainAttribs(const char* linenm,
					    const char* zdomainstr,
					    BufferStringSet& attribs);
    void		getStoredGathersList(bool for3d,BufferStringSet&) const;
    void		storeRlnAs2DLine(const Geometry::RandomLine&) const;

    void		processTime2Depth(bool is2d);
    void		processVelConv() const;
    void		createMultiCubeDataStore() const;

    void		manageSeismics(int opt,bool modal=false);
    void		managePreLoad();
    void		importWavelets();
    void		exportWavelets();
    void		manageWavelets();
    void		exportCubePos(const DBKey* =0);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		ioSeis(int,bool);
    void		survChangedCB(CallBacker*);
    DBKey		getDefault2DDataID() const;

    uiSeisFileMan*	man2dseisdlg_;
    uiSeisFileMan*	man3dseisdlg_;
    uiSeisPreStackMan*	man2dprestkdlg_;
    uiSeisPreStackMan*	man3dprestkdlg_;
    uiSeisWvltMan*	manwvltdlg_;

    uiSeisIOSimple*	imp3dseisdlg_;
    uiSeisIOSimple*	exp3dseisdlg_;
    uiSeisIOSimple*	imp2dseisdlg_;
    uiSeisIOSimple*	exp2dseisdlg_;
    uiSeisIOSimple*	impps3dseisdlg_;
    uiSeisIOSimple*	expps3dseisdlg_;
    uiSeisIOSimple*	impps2dseisdlg_;
    uiSeisIOSimple*	expps2dseisdlg_;
    uiSeisImportODCube*	impodcubedlg_;
    uiSeisImpCubeFromOtherSurveyDlg*	impcubeothsurvdlg_;
    uiSeisExpCubePositionsDlg*		expcubeposdlg_;
	uiSeisWvltImp*				impwvltdlg_;
	uiSeisWvltExp*				expwvltdlg_;
    uiBatchTime2DepthSetup*		t2ddlg2d_;
    uiBatchTime2DepthSetup*		t2ddlg3d_;

private:

    uiString		mkSimpIODlgCaption(bool forread,bool is2d,bool isps);

};
