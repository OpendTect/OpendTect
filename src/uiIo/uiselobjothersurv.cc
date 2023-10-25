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
#include "iodir.h"
#include "ioman.h"
#include "iostrm.h"
#include "oddirs.h"
#include "transl.h"

#include "uimsg.h"
#include "uiselsimple.h"
#include "uisurveyselect.h"


bool getIOObjList( ObjectSet<IOObj>& objs, const SurveyDiskLocation& sdl,
		  const IOObjContext& ctxt, BufferStringSet& nms )
{
    deepErase( objs );
    nms.setEmpty();

    const BufferString datadirnm( sdl.fullPathFor(
			    ctxt.getDataDirName(ctxt.stdseltype_,true) ) );
    const IODir iodir( datadirnm.buf() );
    ObjectSet<IOObj> unsortedobjs;
    for ( int idx=0; idx< iodir.size(); idx++ )
    {
	const IOObj* ioobj = iodir.get( idx );
	if ( !ioobj )
	    continue;

	if ( ctxt.validIOObj(*ioobj) )
	{
	    IOObj* toadd = ioobj->clone();
	    unsortedobjs += toadd;
	    nms.add( toadd->name() );
	}
    }

    if ( nms.isEmpty() )
	return false;

    ConstArrPtrMan<int> idxs = nms.getSortIndexes();
    nms.useIndexes( idxs );
    for ( int idx=0; idx<unsortedobjs.size(); idx++ )
	objs += unsortedobjs[ idxs[idx] ];

    return true;
}


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
}


bool uiSelObjFromOtherSurvey::acceptOK( CallBacker* )
{
    BufferString othersurveyrootdir;
    const SurveyDiskLocation sdl = selfld_->surveyDiskLocation();
    if ( !IOMan::isValidSurveyDir(sdl.fullPath()) )
    {
	const uiRetVal uirv = IOMan::isValidMsg();
	uiMSG().error( tr("Survey doesn't seem to be valid: \n%1").arg(uirv) );
	return false;
    }

    BufferString typenm = ctio_.ctxt_.name();
    if ( typenm.isEmpty() )
	typenm = ctio_.ctxt_.trgroup_->typeName();

    ObjectSet<IOObj> ioobjs;
    BufferStringSet names;
    if ( !getIOObjList(ioobjs,sdl,ctio_.ctxt_,names) )
    {
	uiMSG().error( tr("Survey doesn't have any objects of type '%1'")
			.arg(typenm) );
	return false;
    }

    uiSelectFromList::Setup selsu( uiStrings::phrSelect(toUiString(typenm)),
				   names );
    uiSelectFromList objseldlg( this, selsu );
    if ( !objseldlg.go() )
    {
	deepErase( ioobjs );
	return false;
    }

    const int selidx = objseldlg.selection();
    if ( !ioobjs.validIdx(selidx) )
    {
	deepErase( ioobjs );
	return false;
    }

    IOObj* selobj = ioobjs[selidx];
    mDynamicCastGet(IOStream*,iostrm,selobj);
    if ( iostrm )
	iostrm->fileSpec().ensureBaseDir( sdl.fullPath() );

    ctio_.setObj( selobj->clone() );
    ctio_.setName( selobj->name() );
    fulluserexpression_ = selobj->fullUserExpr();
    deepErase( ioobjs );
    return true;
}


SurveyDiskLocation uiSelObjFromOtherSurvey::getSurveyDiskLocation() const
{
    return selfld_->surveyDiskLocation();
}


void uiSelObjFromOtherSurvey::setDirToCurrentSurvey()
{
    selfld_->setSurveyDiskLocation( SurveyDiskLocation::currentSurvey() );
}


void uiSelObjFromOtherSurvey::setDirToOtherSurvey()
{
}


void uiSelObjFromOtherSurvey::setDirToOtherSurvey(
					const SurveyDiskLocation& sdl )
{
    selfld_->setSurveyDiskLocation( sdl );
}
