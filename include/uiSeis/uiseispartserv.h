#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiapplserv.h"
#include "multiid.h"
#include "uistring.h"

class BufferStringSet;
class TrcKeyZSampling;
class IOPar;
class SeisTrcBuf;
class uiBatchTime2DepthSetup;
class uiFlatViewWin;
class uiSeisFileMan;
class uiSeisImportCBVS;
class uiSeisIOSimple;
class uiSeisImpCBVSFromOtherSurveyDlg;
class uiSeisExpCubePositionsDlg;
class uiSeisPreStackMan;
class uiSeisWvltExp;
class uiSeisWvltImp;
class uiSeisWvltMan;

namespace PosInfo { class Line2DData; }
namespace Geometry { class RandomLine; }

/*!
\brief Seismic User Interface Part Server
*/

mExpClass(uiSeis) uiSeisPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiSeisPartServer)
public:
			uiSeisPartServer(uiApplService&);
			~uiSeisPartServer();

    const char*		name() const override		{ return "Seismics"; }

    bool		importSeis(int opt);
    bool		exportSeis(int opt);

    MultiID		getDefaultDataID(bool is2d) const;
    bool		select2DSeis(MultiID&);
    bool		select2DLines(TypeSet<Pos::GeomID>&,int& action);
    static void		get2DStoredAttribs(const char* linenm,
				       BufferStringSet& attribs,int steerpol=2);
    void		get2DZdomainAttribs(const char* linenm,
					    const char* zdomainstr,
					    BufferStringSet& attribs);
    bool		create2DOutput(const MultiID&,const char* linekey,
				       TrcKeyZSampling&,SeisTrcBuf&);
    void		getStoredGathersList(bool for3d,BufferStringSet&) const;
    void		storeRlnAs2DLine(const Geometry::RandomLine&) const;

    void		processTime2Depth(bool is2d=false) const;
    void		processVelConv() const;
    void		createMultiCubeDataStore() const;

    void		manageSeismics(int opt,bool modal=false);
    void		managePreLoad();
    void		importWavelets();
    void		exportWavelets();
    void		manageWavelets();
    void		exportCubePos(const MultiID* =0);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		ioSeis(int,bool);
    void		survChangedCB(CallBacker*);
    MultiID		getDefault2DDataID() const;

    uiBatchTime2DepthSetup* t2ddlgs2d_ = nullptr;
    uiBatchTime2DepthSetup* t2ddlgs3d_ = nullptr;

    uiSeisFileMan*	man2dseisdlg_ = nullptr;
    uiSeisFileMan*	man3dseisdlg_ = nullptr;
    uiSeisPreStackMan*	man2dprestkdlg_ = nullptr;
    uiSeisPreStackMan*	man3dprestkdlg_ = nullptr;
    uiSeisWvltMan*	manwvltdlg_ = nullptr;
    uiSeisWvltImp*	impwvltdlg_ = nullptr;
    uiSeisWvltExp*	expwvltdlg_ = nullptr;

    uiSeisIOSimple*	imp3dseisdlg_ = nullptr;
    uiSeisIOSimple*	exp3dseisdlg_ = nullptr;
    uiSeisIOSimple*	imp2dseisdlg_ = nullptr;
    uiSeisIOSimple*	exp2dseisdlg_ = nullptr;
    uiSeisIOSimple*	impps3dseisdlg_ = nullptr;
    uiSeisIOSimple*	expps3dseisdlg_ = nullptr;
    uiSeisIOSimple*	impps2dseisdlg_ = nullptr;
    uiSeisIOSimple*	expps2dseisdlg_ = nullptr;
    uiSeisImportCBVS*	impcbvsdlg_ = nullptr;
    uiSeisImpCBVSFromOtherSurveyDlg* impcbvsothsurvdlg_ = nullptr;
    uiSeisExpCubePositionsDlg*	expcubeposdlg_ = nullptr;

private:
    uiString		mkDlgCaption( bool forread, bool is2d, bool isps );
};
