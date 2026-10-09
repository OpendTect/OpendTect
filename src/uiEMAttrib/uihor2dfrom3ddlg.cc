/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihor2dfrom3ddlg.h"

#include "bufstringset.h"
#include "emmanager.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
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
    : uiDialog(p,Setup(uiStrings::phrCreate(tr("%1 from %2")
			  .arg(uiStrings::phrJoinStrings(uiStrings::s2D(),
			  uiStrings::sHorizon().toLower()))
			  .arg(uiStrings::s3D())),
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


uiHor2DFrom3DDlg::~uiHor2DFrom3DDlg()
{}


bool uiHor2DFrom3DDlg::acceptOK( CallBacker* )
{
    BufferStringSet outnms;
    if ( !checkFlds() || !checkOutNames(outnms) )
	return false;

    uiTaskRunner runner( this );

    TypeSet<Pos::GeomID> geomids;
    linesetinpsel_->getSelGeomIDs( geomids );
    const TypeSet<MultiID>& horids = hor3dselfld_->getSelected();

    EM::EMManager& em = EM::EMM();
    emobjids_.setEmpty();
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	const MultiID mid = horids[idx];
	RefMan<EM::EMObject> em3dobj = em.loadIfNotFullyLoaded( mid, &runner );
	mDynamicCastGet(EM::Horizon3D*,hor3d, em3dobj.ptr());
	if ( !hor3d )
	    continue;

	EM::ObjectID emid = em.createObject( EM::Horizon2D::typeStr(),
					     outnms[idx]->buf() );
	RefMan<EM::EMObject> em2dobj = em.getObject(emid);
	mDynamicCastGet(EM::Horizon2D*,hor2d,em2dobj.ptr());
	if ( !hor2d )
	    continue;

	PtrMan<Hor2DFrom3DCreatorGrp> creator = new Hor2DFrom3DCreatorGrp(
							    *hor3d, *hor2d );
	hor2d->setPreferredColor( hor3d->preferredColor() );
	hor2d->setPreferredLineStyle( hor3d->preferredLineStyle() );
	creator->init( geomids );
	if ( !runner.execute(*creator.ptr()) )
	{
	    uiMSG().errorWithDetails( creator->uiMessage(),
			tr("Failed to create 2D Horizon from 3D") );
	    return false;
	}

	PtrMan<Task> saver = hor2d->saver();
	if ( !runner.execute(*saver.ptr()) )
	{
	    uiMSG().errorWithDetails( saver->uiMessage(),
			tr("Failed to save 2D Horizon") );
	    return false;
	}

	if ( doDisplay() )
	{
	    emobjids_ += emid;
	    em2dobj->unRefNoDelete(); //Should not be needed
	}
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
	if ( IOM().get(hornm,mTranslGroupName(EMHorizon2D)) )
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
