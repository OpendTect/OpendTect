/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016
________________________________________________________________________

-*/

#include "uiselobjothersurv.h"

#include "ctxtioobj.h"
#include "uiioobjseldlg.h"
#include "uisurveyselect.h"


uiSelObjFromOtherSurvey::uiSelObjFromOtherSurvey( uiParent* p,
						  const IOObjContext& ctxt )
    : parent_(p)
    , ctio_(*new CtxtIOObj(ctxt))
{
    ctio_.setObj( 0 );
}


uiSelObjFromOtherSurvey::~uiSelObjFromOtherSurvey()
{
    ctio_.setObj( 0 );
    delete &ctio_;
}


const IOObj* uiSelObjFromOtherSurvey::ioObj() const
{
    return ctio_.ioobj_;
}


bool uiSelObjFromOtherSurvey::go()
{
    uiSurveySelectDlg survseldlg( parent_ );
    if ( !survseldlg.go() )
	return false;

    uiIOObjSelDlg::Setup su;
    su.survdir( survseldlg.getSurveyPath() );
    uiIOObjSelDlg objseldlg( &survseldlg, su, ctio_.ctxt_ );
    if ( !objseldlg.go() )
	return false;
    const IOObj* ioobj = objseldlg.ioObj();
    if ( !ioobj )
	return false;

    ctio_.setObj( ioobj->clone() );
    usrexpr_ = ioobj->fullUserExpr();
    return true;
}
