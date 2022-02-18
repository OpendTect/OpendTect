/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2008
________________________________________________________________________

-*/

#include "uihor2dfrom3ddlg.h"

#include "bufstringset.h"
#include "emmanager.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "hor2dfrom3dcreator.h"
#include "ioman.h"

#include "uigeninput.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uitaskrunner.h"
#include "uibutton.h"
#include "od_helpids.h"


uiHor2DFrom3DDlg::uiHor2DFrom3DDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrCreate(tr("%1 from %2")
			  .arg(uiStrings::phrJoinStrings(uiStrings::s2D(),
			  uiStrings::sHorizon().toLower()))
			  .arg(uiStrings::s3D())), mNoDlgTitle,
			  mODHelpKey(mHor2DFrom3DDlgHelpID)))
{
    hor3dselfld_ = new uiHorizonParSel( this, false, true );

    linesetinpsel_ = new uiSeis2DMultiLineSel( this );
    linesetinpsel_->attach( alignedBelow, hor3dselfld_ );

    hor2dnmfld_ = new uiGenInput( this, tr("%1 name prefix")
				     .arg(mJoinUiStrs(sHorizon(),s2D())) );
    hor2dnmfld_->attach( alignedBelow, linesetinpsel_ );

    displayfld_ = new uiCheckBox( this, tr("Display on OK") );
    displayfld_->setChecked( true );
    displayfld_->attach( alignedBelow,hor2dnmfld_ );
}


bool uiHor2DFrom3DDlg::acceptOK( CallBacker* )
{
    BufferStringSet outnms;
    if ( !checkFlds() || !checkOutNames(outnms) )
	return false;

    uiTaskRunner uitr( this );

    TypeSet<Pos::GeomID> geomids;
    linesetinpsel_->getSelGeomIDs( geomids );
    const TypeSet<MultiID>& horids = hor3dselfld_->getSelected();

    EM::EMManager& em = EM::EMM();
    emobjids_.setEmpty();
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	const MultiID mid = horids[idx];
	RefMan<EM::EMObject> emobj = em.loadIfNotFullyLoaded( mid, &uitr );
	mDynamicCastGet(EM::Horizon3D*,hor3d, emobj.ptr());
	if ( !hor3d )
	    continue;
	hor3d->ref();

	EM::ObjectID emid = em.createObject( EM::Horizon2D::typeStr(),
					     outnms[idx]->buf() );
	EM::EMObject* em2dobj = em.getObject(emid);
	mDynamicCastGet(EM::Horizon2D*,hor2d,em2dobj);
	if ( !hor2d )
	    continue;
	hor2d->ref();

	PtrMan<Hor2DFrom3DCreatorGrp> creator = new Hor2DFrom3DCreatorGrp(
							    *hor3d, *hor2d );
	hor2d->setPreferredColor( hor3d->preferredColor() );
	hor2d->setPreferredLineStyle( hor3d->preferredLineStyle() );
	creator->init( geomids );
	creator->add( hor2d->saver() );
	TaskRunner::execute( &uitr, *creator );

	if ( doDisplay() )
	    emobjids_ += emid;
	else
	    hor2d->unRef();
	hor3d->unRef();
    }

    return true;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiHor2DFrom3DDlg::checkFlds()
{
    if ( !hor3dselfld_->getSelected().size() )
	mErrRet( tr("Please select at least one 3D Horizon. ") )
    if ( !linesetinpsel_->nrSelected() )
	mErrRet( tr("Please select at least one 2D line") )

    return true;
}


bool uiHor2DFrom3DDlg::checkOutNames( BufferStringSet& outnms ) const
{
    outnms.setEmpty();
    const TypeSet<MultiID>& horids = hor3dselfld_->getSelected();
    BufferStringSet existinghornms;
    for ( const auto& mid : horids )
    {
	BufferString hornm( hor2dnmfld_->text(), EM::EMM().objectName(mid) );
	IOM().to( IOObjContext::Surf );
	if ( IOM().getLocal(hornm,0) )
	    existinghornms.add( hornm );

	outnms.add( hornm );
    }

    if ( !existinghornms.isEmpty() )
    {
	const bool ret = uiMSG().askGoOn(tr("Horizons %1 already exist. "
					    "Do You want to overwrite them.")
	    .arg(toUiString(existinghornms.getDispString(10))));
	if ( !ret )
	{
	    outnms.setEmpty();
	    return false;
	}
    }

    return true;
}


bool uiHor2DFrom3DDlg::doDisplay() const
{
    return displayfld_->isChecked();
}
