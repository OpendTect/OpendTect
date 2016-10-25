/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016
________________________________________________________________________

-*/

#include "uiselobjothersurv.h"

#include "ioobjctxt.h"
#include "uimsg.h"


#include "uilabel.h"

uiSelObjFromOtherSurvey::uiSelObjFromOtherSurvey( uiParent* p,
						  const IOObjContext& ctxt )
    : uiDialog(p,Setup(tr("Select %1").arg(ctxt.objectTypeName()),
			mNoDlgTitle,mODHelpKey(mSelObjFromOtherSurveyHelpID)))
    , ctxt_(*new IOObjContext(ctxt))
{
    new uiLabel( this, mTODONotImplPhrase() );
}


uiSelObjFromOtherSurvey::~uiSelObjFromOtherSurvey()
{
}


const IOObj* uiSelObjFromOtherSurvey::ioObj() const
{
    return 0;
}


bool uiSelObjFromOtherSurvey::acceptOK()
{
    return true;
}
