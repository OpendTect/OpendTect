/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016
________________________________________________________________________

-*/

#include "uiselobjothersurv.h"

#include "uisurveyselect.h"
#include "uilistbox.h"
#include "dbdir.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "filepath.h"
#include "uimsg.h"


uiSelObjFromOtherSurvey::uiSelObjFromOtherSurvey( uiParent* p,
						  const IOObjContext& ctxt )
    : uiDialog(p,Setup(tr("Select %1").arg(ctxt.objectTypeName()),
			mNoDlgTitle,mODHelpKey(mSelObjFromOtherSurveyHelpID)))
    , ctxt_(*new IOObjContext(ctxt))
{
    survsel_ = new uiSurveySelect( this );

    objfld_ = new uiListBox( this, "Objects" );
    objfld_->setHSzPol( uiObject::WideVar );
    objfld_->setStretch( 2, 2 );
    objfld_->attach( alignedBelow, survsel_ );

    mAttachCB( survsel_->survDirChg, uiSelObjFromOtherSurvey::survSelCB );
    mAttachCB( objfld_->doubleClicked, uiSelObjFromOtherSurvey::accept );
    mAttachCB( postFinalise(), uiSelObjFromOtherSurvey::survSelCB );
}


uiSelObjFromOtherSurvey::~uiSelObjFromOtherSurvey()
{
    deepErase( ioobjs_ );
}


void uiSelObjFromOtherSurvey::survSelCB( CallBacker* )
{
    File::Path fp( ctxt_.getDataDirName(ctxt_.stdseltype_) );
    fp.setPath( survsel_->getFullDirPath() );
    const BufferString datadirnm( fp.fullPath() );
    ConstRefMan<DBDir> dbdir = new DBDir( datadirnm );
    BufferStringSet objnms;
    deepErase( ioobjs_ );
    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( ctxt_.validIOObj(ioobj) )
	{
	    IOObj* toadd = ioobj.clone();
	    toadd->setAbsDirectory( datadirnm );
	    ioobjs_ += toadd;
	    objnms.add( toadd->name() );
	}
    }

    objfld_->setEmpty();
    objfld_->addItems( objnms );
    if ( !objnms.isEmpty() )
	objfld_->setCurrentItem( 0 );
}



BufferString uiSelObjFromOtherSurvey::sourceSurveyDirectory() const
{
    return survsel_->getFullDirPath();
}


const IOObj* uiSelObjFromOtherSurvey::ioObj() const
{
    return ioobjs_.validIdx(selidx_) ? ioobjs_[selidx_] : 0;
}


bool uiSelObjFromOtherSurvey::acceptOK()
{
    selidx_ = objfld_->currentItem();
    const IOObj* ioobj = ioObj();
    if ( !ioobj )
	return false;

    usrexpr_.set( ioobj->fullUserExpr() );
    return true;
}
