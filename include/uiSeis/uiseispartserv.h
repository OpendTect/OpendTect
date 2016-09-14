#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiapplserv.h"
#include "dbkey.h"
#include "uistring.h"

class BufferStringSet;
class TrcKeyZSampling;
class IOPar;
class SeisTrcBuf;
class uiFlatViewWin;
class uiSeisFileMan;
class uiSeisImportCBVS;
class uiSeisIOSimple;
class uiSeisImpCBVSFromOtherSurveyDlg;
class uiSeisPreStackMan;
class uiSeisWvltMan;

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
    bool		select2DLines(TypeSet<Pos::GeomID>&,int& action);
    static void		get2DStoredAttribs(const char* linenm,
				       BufferStringSet& attribs,int steerpol=2);
    void		get2DZdomainAttribs(const char* linenm,
					    const char* zdomainstr,
					    BufferStringSet& attribs);
    bool		create2DOutput(const DBKey&,const char* linekey,
				       TrcKeyZSampling&,SeisTrcBuf&);
    void		getStoredGathersList(bool for3d,BufferStringSet&) const;
    void		storeRlnAs2DLine(const Geometry::RandomLine&) const;

    void		processTime2Depth() const;
    void		processVelConv() const;
    void		createMultiCubeDataStore() const;

    void		manageSeismics(int opt,bool modal=false);
    void		managePreLoad();
    void		importWavelets();
    void		exportWavelets();
    void		manageWavelets();

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
    uiSeisImportCBVS*	impcbvsdlg_;
    uiSeisImpCBVSFromOtherSurveyDlg* impcbvsothsurvdlg_;

private:
    uiString		mkDlgCaption( bool forread, bool is2d, bool isps );
};

