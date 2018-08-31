/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
________________________________________________________________________

-*/


#include "ui2dgeomman.h"

#include "bufstringset.h"
#include "dbman.h"
#include "file.h"
#include "od_helpids.h"
#include "posinfo2dsurv.h"
#include "survgeom2d.h"
#include "survgeometrytransl.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiimpexp2dgeom.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitable.h"

static IOObjContext mkCtxt()
{
    IOObjContext ret( mIOObjContext(SurvGeom2D) );
    return ret;
}


ui2DGeomManageDlg::ui2DGeomManageDlg( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage( tr("2D Geometry")),
			       mNoDlgTitle, mODHelpKey(m2DGeomManageDlgHelpID))
			       .nrstatusflds(1).modal(false),mkCtxt())
{
    createDefaultUI( false, false );
    selgrp_->getManipGroup()->addButton( "delete", tr("Delete this line"),
		     mCB(this,ui2DGeomManageDlg,lineRemoveCB) );
    selgrp_->getManipGroup()->addButton( "browse2dgeom",
						    tr("Manage Line Geometry"),
				mCB(this,ui2DGeomManageDlg,manLineGeom) );
}


ui2DGeomManageDlg::~ui2DGeomManageDlg()
{
}


//-----------Manage Line Geometry-----------------

class uiManageLineGeomDlg : public uiDialog
{ mODTextTranslationClass(uiManageLineGeomDlg)
public:

uiManageLineGeomDlg( uiParent* p, const char* linenm, bool readonly )
    : uiDialog(p,uiDialog::Setup( tr("Manage Line Geometry"),mNoDlgTitle,
				  mODHelpKey(mManageLineGeomDlgHelpID)))
    , linenm_(linenm),readonly_(readonly)
{
    if ( readonly )
    {
	setCtrlStyle( CloseOnly );
	setCaption( tr("Browse Line Geometry") );
    }

    uiString lbl;
    lbl = toUiString("%1: %2").arg(uiStrings::sLineName())
						.arg(toUiString(linenm));
    uiLabel* titllbl = new uiLabel( this, lbl );
    titllbl->attach( hCentered );

    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(linenm) )
    if ( !geom2d )
    {
	uiMSG().error(tr("Cannot find geometry for %1").arg(linenm));
	return;
    }

    const TypeSet<PosInfo::Line2DPos>& positions = geom2d->data().positions();
    table_ = new uiTable( this, uiTable::Setup(positions.size(),3), "2DGeom" );
    table_->attach( ensureBelow, titllbl );
    table_->setPrefWidth( 400 );
    uiStringSet collbls;
    collbls.add( uiStrings::sTraceNumber() );
    collbls.add( uiStrings::sSPNumber() );
    collbls.add( uiStrings::sX() );
    collbls.add( uiStrings::sY() );
    table_->setColumnLabels( collbls );
    if ( readonly )
	table_->setTableReadOnly( true );

    FloatInpIntervalSpec spec( true );
    rgfld_ = new uiGenInput( this, uiStrings::sZRange(), spec );
    rgfld_->attach( leftAlignedBelow, table_ );
    rgfld_->setValue( geom2d->data().zRange() );
    rgfld_->setReadOnly( readonly );

    if ( !readonly )
    {
	readnewbut_ = new uiPushButton( this, tr("Set new Geometry"),
			mCB(this,uiManageLineGeomDlg,impLineGeom), false );
	readnewbut_->attach( centeredBelow, rgfld_ );
    }

    fillTable( *geom2d );
}


void impLineGeom( CallBacker* )
{
    if ( readonly_ )
	return;

    uiImp2DGeom dlg( this, linenm_ );
    if ( !dlg.go() ) return;

    RefMan<Survey::Geometry2D> geom = new Survey::Geometry2D( linenm_ );
    if ( !dlg.fillGeom(*geom) )
	return;

    table_->clearTable();
    fillTable( *geom );
}


void fillTable( const Survey::Geometry2D& geom2d )
{
    const TypeSet<PosInfo::Line2DPos>& positions = geom2d.data().positions();
    const TypeSet<int>& spnrs = geom2d.spnrs();
    table_->setNrRows( positions.size() );
    for ( int idx=0; idx<positions.size(); idx++ )
    {
	table_->setValue( RowCol(idx,0), positions[idx].nr_ );
	table_->setValue( RowCol(idx,1), spnrs.validIdx(idx) ? spnrs[idx] : -1);
	table_->setValue( RowCol(idx,2), positions[idx].coord_.x_ );
	table_->setValue( RowCol(idx,3), positions[idx].coord_.y_ );
    }
}


bool acceptOK()
{
    if (!uiMSG().askGoOn(tr("Do you really want to change the geometry?\n"
			    "This will affect all associated data.")))
	return false;

    Pos::GeomID geomid = Survey::GM().getGeomID( linenm_ );
    mDynamicCastGet(Survey::Geometry2D*,geom2d,
		    Survey::GMAdmin().getGeometry(geomid) )
    if ( !geom2d )
	return true;

    geom2d->setEmpty();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	geom2d->add( table_->getDValue(RowCol(idx,2)),
		     table_->getDValue(RowCol(idx,3)),
		     table_->getIntValue(RowCol(idx,0)),
		     table_->getIntValue(RowCol(idx,1)) );
    }

    geom2d->dataAdmin().setZRange( rgfld_->getFStepInterval() );
    geom2d->touch();

    uiString errmsg;
    if ( !Survey::GMAdmin().write(*geom2d,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }
    return true;
}

    const char*		linenm_;
    bool		readonly_;
    uiTable*		table_;
    uiGenInput*		rgfld_;
    uiPushButton*	readnewbut_;
};


//-----------------------------------------------------


void ui2DGeomManageDlg::manLineGeom( CallBacker* )
{
    if ( !curioobj_ ) return;

    PtrMan<Translator> transl = curioobj_->createTranslator();
    if ( !transl )
	return;

    const BufferString linenm = curioobj_->name();
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(linenm) )
    if ( !geom2d )
    {
	uiMSG().error(tr("Cannot find geometry for %1").arg(linenm));
	return;
    }

    uiManageLineGeomDlg dlg( this, linenm,
			     !transl->isUserSelectable(false) );
    dlg.go();
}


void ui2DGeomManageDlg::ownSelChg()
{
}


bool ui2DGeomManageDlg::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    return File::exists( ioobj.mainFileName() );
}


void ui2DGeomManageDlg::lineRemoveCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const bool docont = uiMSG().askContinue(
       tr("All selected 2D line geometries will be deleted.\n"
	  "This will invalidate all data and interpretations associated with "
	  "these lines"));
    if ( !docont )
	return;

    uiUserShowWait usw( this, uiStrings::sUpdatingDB() );
    uiStringSet msgs;
    DBKeySet selids;
    selgrp_->getChosen( selids );
    for ( int idx=0; idx<selids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = DBM().get( selids[idx] );
	if ( !ioobj || ioobj->implReadOnly() )
	    continue;

	const BufferString lnm( ioobj->name() );
	Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	    return;

	if ( !fullImplRemove(*ioobj) )
	{
	    msgs += tr("Cannot remove %1").arg(lnm);
	    continue;
	}

	DBM().removeEntry( ioobj->key() );
	Survey::GMAdmin().removeGeometry( geomid );
	const FixedString crfromstr = ioobj->pars().find( sKey::CrFrom() );
	if ( !crfromstr.isEmpty() )
	{
	    PosInfo::Line2DKey l2dkey;
	    l2dkey.fromString( crfromstr );
	    if ( l2dkey.isOK() )
	    {
		PosInfo::Survey2D& old2dadmin = PosInfo::POS2DAdmin();
		if ( old2dadmin.curLineSetID() != l2dkey.lsID() )
		    old2dadmin.setCurLineSet( l2dkey.lsID() );

		old2dadmin.removeLine( l2dkey.lineID() );
	    }
	}
    }

    usw.readyNow();
    selgrp_->fullUpdate( DBKey::getInvalid() );

    if ( !msgs.isEmpty() )
	uiMSG().errorWithDetails(msgs);
}
