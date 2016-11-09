/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "envvars.h"
#include "mmbatchjobdispatch.h"
#include "oddirs.h"
#include "settings.h"
#include "uibatchjobdispatcherlauncher.h"
#include "uibatchlaunch.h"
#include "uiclusterjobprov.h"
#include "uicoordsystem.h"
#include "uimsg.h"
#include "plugins.h"
#include "uiposprovgroupstd.h"
#include "uiposfiltgroupstd.h"
#include "ui2dsip.h"
#include "uisurveyselect.h"
#include "uisurvinfoed.h"
#include "survinfo.h"


static const char* sKeyClusterProc = "dTect.Enable Cluster Processing";
static const char* sKeyClusterProcEnv = "DTECT_CLUSTER_PROC";


class uiMMBatchJobDispatcherLauncher : public uiBatchJobDispatcherLauncher
{ mODTextTranslationClass(uiMMBatchJobDispatcherLauncher)
public:

uiMMBatchJobDispatcherLauncher( Batch::JobSpec& js )
    : uiBatchJobDispatcherLauncher(js) {}

mDefaultFactoryInstantiation1Param(uiBatchJobDispatcherLauncher,
		       uiMMBatchJobDispatcherLauncher,
		       Batch::JobSpec&,"Multi-Machine",tr("Multi-Job/Machine"));

    virtual Batch::JobDispatcher&	gtDsptchr() { return jd_; }
    virtual bool			go(uiParent*);
    Batch::MMJobDispatcher		jd_;

};


bool uiMMBatchJobDispatcherLauncher::go( uiParent* p )
{
    const HostDataList hdl( false );
    const HostData* localhost = hdl.find( HostData::localHostName() );
    if ( !localhost )
    {
	if ( hdl.isEmpty() )
	    return uiBatchJobDispatcherLauncher::go( p );

	uiMSG().error( tr("Cannot find configuration for localhost") );
	return false;
    }

    const File::Path localbasedatadir( GetBaseDataDir() );
    if ( localbasedatadir != localhost->getDataRoot() )
    {
	uiMSG().error( tr("Current Data Root: '%1'\ndoes not match path in "
			  "batch processing configuration file:\n'%2'")
			  .arg( localbasedatadir.fullPath() )
			  .arg( localhost->getDataRoot().fullPath() ) );
	return false;
    }

    return uiBatchJobDispatcherLauncher::go( p );
}


static bool enabClusterProc()
{
    bool enabclusterproc = false;
    const bool hassetting =
	Settings::common().getYN( sKeyClusterProc, enabclusterproc );
    if ( !hassetting )
	enabclusterproc = GetEnvVarYN( sKeyClusterProcEnv );
    return enabclusterproc;
}


mExpClass(uiIo) uiCopySurveySIP : public uiSurvInfoProvider
{ mODTextTranslationClass(uiCopySurveySIP)
public:
			uiCopySurveySIP()   {}

    virtual uiString	usrText() const
			{ return tr("Copy from other survey"); }

    virtual uiDialog*	dialog(uiParent*,TDInfo);
    virtual bool	getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    virtual const char*	iconName() const    { return "copyobj"; }

    virtual TDInfo	tdInfo( bool& known ) const
			{ known = tdinfknown_; return tdinf_; }
    virtual bool	xyInFeet() const    { return xyinft_; }

protected:

    TDInfo		tdinf_;
    bool		tdinfknown_;
    bool		xyinft_;

};

class uiSurveyToCopyDlg : public uiDialog
{ mODTextTranslationClass(uiSurveyToCopyDlg)
public:

uiSurveyToCopyDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Select Survey to duplicate Setup from"),
		mNoDlgTitle,mODHelpKey(mCopySurveySIPHelpID)))
{
    survsel_ = new uiSurveySelect( this );
}

    uiSurveySelect* survsel_;

};


uiDialog* uiCopySurveySIP::dialog( uiParent* p, TDInfo )
{
    return new uiSurveyToCopyDlg( p );
}


bool uiCopySurveySIP::getInfo( uiDialog* dlg, TrcKeyZSampling& cs, Coord crd[3])
{
    tdinf_ = Time; xyinft_ = false; tdinfknown_ = false;
    mDynamicCastGet(uiSurveyToCopyDlg*,seldlg,dlg)
    if ( !seldlg )
	return false;

    const BufferString survdir = seldlg->survsel_->getFullDirPath();
    uiRetVal uirv;
    PtrMan<SurveyInfo> survinfo = SurveyInfo::read( survdir, uirv );
    if ( !survinfo )
	return false;

    cs = survinfo->sampling( false );
    crd[0] = survinfo->transform( cs.hsamp_.start_ );
    crd[1] = survinfo->transform( cs.hsamp_.stop_ );
    crd[2] = survinfo->transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()));

    tdinf_ = survinfo->zIsTime() ? Time
				 : (survinfo->zInFeet() ? DepthFeet : Depth);
    xyinft_ = survinfo->xyInFeet();
    tdinfknown_ = true;

    return true;
}


mDefModInitFn(uiIo)
{
    mIfNotFirstTime( return );

    uiRangePosProvGroup::initClass();
    uiPolyPosProvGroup::initClass();
    uiTablePosProvGroup::initClass();

    uiRandPosFiltGroup::initClass();
    uiSubsampPosFiltGroup::initClass();

    uiMMBatchJobDispatcherLauncher::initClass();
    Batch::ClusterJobDispatcher::addDef( new Batch::SimpleClusterProgDef );
    if ( enabClusterProc() )
	uiClusterJobDispatcherLauncher::initClass();

    uiProcSettings::initClass();
    Coords::uiUnlocatedXYSystem::initClass();

    uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );
    uiSurveyInfoEditor::addInfoProvider( new uiCopySurveySIP );
}
