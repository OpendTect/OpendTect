/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Dec 2010
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
    othersurveyrootdir_.setEmpty();
}


uiSelObjFromOtherSurvey::~uiSelObjFromOtherSurvey()
{
    ctio_.setObj( 0 );
    setDirToCurrentSurvey();
}


bool uiSelObjFromOtherSurvey::acceptOK( CallBacker* )
{
    if ( !selfld_->getFullSurveyPath(othersurveyrootdir_) )
	return false;

    if ( !File::exists( othersurveyrootdir_ ) )
    {
	othersurveyrootdir_.setEmpty();
	uiMSG().error( tr("Survey doesn't seem to exist") );
	return false;
    }

    setDirToOtherSurvey();
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
	    iostrm->fileSpec().ensureBaseDir( othersurveyrootdir_ );
	fulluserexpression_ = ctio_.ioobj_->fullUserExpr();
	success = true;
    }
    ctio_.ctxt_.forread_ = prevctiostate;
    return success;
}


void uiSelObjFromOtherSurvey::setDirToCurrentSurvey()
{
    IOM().setRootDir( GetDataDir() );
}


void uiSelObjFromOtherSurvey::setDirToOtherSurvey()
{
    if ( !othersurveyrootdir_.isEmpty() )
	IOM().setRootDir( othersurveyrootdir_ );
}
