/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiselobjothersurv.h"

#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iostrm.h"
#include "oddirs.h"

#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uisurveyselect.h"


uiSelObjFromOtherSurvey::uiSelObjFromOtherSurvey( uiParent* p, CtxtIOObj& ctio )
    : uiDialog(p,Setup(tr("Select survey"),mNoDlgTitle,
		       mODHelpKey(mSelObjFromOtherSurveyHelpID)))
    , ctio_(ctio)
{
    selfld_ = new uiSurveySelect( this, true, true );
}


uiSelObjFromOtherSurvey::~uiSelObjFromOtherSurvey()
{
    ctio_.setObj( nullptr );
    delete changer_;
}


bool uiSelObjFromOtherSurvey::acceptOK( CallBacker* )
{
    BufferString othersurveyrootdir;
    if ( !selfld_->getFullSurveyPath(othersurveyrootdir) )
	return false;

    const uiRetVal uirv = IOMan::isValidSurveyDir( othersurveyrootdir.buf() );
    if ( !uirv.isOK() )
    {
	uiMSG().error( tr("Survey doesn't seem to be valid: \n%1").arg(uirv) );
	return false;
    }

    const FilePath fp( othersurveyrootdir.buf() );
    const SurveyDiskLocation sdl( fp );
    setDirToOtherSurvey( sdl );
    bool prevctiostate = ctio_.ctxt_.forread_;
    ctio_.ctxt_.forread_ = true;
    uiIOObjSelDlg objdlg( this, ctio_ );
    bool success = false;
    if ( objdlg.go() && objdlg.ioObj() )
    {
	ctio_.setObj( objdlg.ioObj()->clone() );
	ctio_.setName( ctio_.ioobj_->name() );
	mDynamicCastGet(IOStream*,iostrm,ctio_.ioobj_);
	if ( iostrm )
	    iostrm->fileSpec().ensureBaseDir( othersurveyrootdir.buf() );
	fulluserexpression_ = ctio_.ioobj_->fullUserExpr();
	success = true;
    }

    ctio_.ctxt_.forread_ = prevctiostate;
    return success;
}


void uiSelObjFromOtherSurvey::setDirToCurrentSurvey()
{
    deleteAndNullPtr( changer_ );
}


void uiSelObjFromOtherSurvey::setDirToOtherSurvey(
						const SurveyDiskLocation& sdl )
{
    delete changer_;
    changer_ = new SurveyChanger( sdl );
}
