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
#include "linear.h"
#include "od_helpids.h"
#include "posinfo2dsurv.h"
#include "survgeom2d.h"
#include "survgeometrytransl.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
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
    selgrp_->getManipGroup()->addButton( "delete",
				tr("Delete this Line"),
				mCB(this,ui2DGeomManageDlg,lineRemoveCB) );
    selgrp_->getManipGroup()->addButton( "browse2dgeom",
				tr("Manage Line Geometry"),
				mCB(this,ui2DGeomManageDlg,manLineGeom) );
}


ui2DGeomManageDlg::~ui2DGeomManageDlg()
{
}


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

    uiManageLineGeomDlg dlg( this, geom2d->id(),
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



// uiTrc2SPDlg
class uiTrc2SPDlg : public uiDialog
{ mODTextTranslationClass(uiTrc2SPDlg)
public:
uiTrc2SPDlg( uiParent* p )
	: uiDialog(p,Setup(tr("Set Trace Number vs SP Number Relationship" ),
			   mNoDlgTitle,mTODOHelpKey))
{
    dirfld_ = new uiGenInput( this, tr("Calculate"),
	BoolInpSpec(true,uiStrings::sSPNumber(),uiStrings::sTraceNumber()) );
    dirfld_->valuechanged.notify( mCB(this,uiTrc2SPDlg,dirChg) );

    uiString splbl = toUiString( "%1 =" ).arg( uiStrings::sSPNumber() );
    spincrfld_ = new uiGenInput( this, splbl, FloatInpSpec(1) );
    spincrfld_->attach( alignedBelow, dirfld_ );
    spstartfld_ = new uiGenInput( this, toUiString("x TrcNr +"),
				  FloatInpSpec(0) );
    spstartfld_->attach( rightTo, spincrfld_ );

    uiString trclbl = toUiString( "%1 =" ).arg( uiStrings::sTraceNumber() );
    trcincrfld_ = new uiGenInput( this, trclbl, FloatInpSpec(1) );
    trcincrfld_->attach( alignedBelow, dirfld_ );
    trcstartfld_ = new uiGenInput( this, toUiString("x SP +"),
				   FloatInpSpec(0) );
    trcstartfld_->attach( rightTo, trcincrfld_ );

    dirChg( 0 );
}



bool calcSP() const
{
    return dirfld_->getBoolValue();
}


LinePars getRelationship() const
{
    return calcSP()
	? LinePars( spstartfld_->getFValue(), spincrfld_->getFValue() )
	: LinePars( trcstartfld_->getFValue(), trcincrfld_->getFValue() );
}


protected:
void dirChg( CallBacker* )
{
    const bool calcsp = calcSP();
    spincrfld_->display( calcsp );
    spstartfld_->display( calcsp );
    trcincrfld_->display( !calcsp );
    trcstartfld_->display( !calcsp );
}


bool acceptOK()
{
    bool isudf = false;
    if ( calcSP() )
	isudf = spstartfld_->isUndef() || spincrfld_->isUndef();
    else
	isudf = trcstartfld_->isUndef() || trcincrfld_->isUndef();

    if ( isudf )
    {
	uiMSG().error( tr("Please enter valid relationship.") );
	return false;
    }

    return true;
}

    uiGenInput*		dirfld_;
    uiGenInput*		trcstartfld_;
    uiGenInput*		trcincrfld_;
    uiGenInput*		spstartfld_;
    uiGenInput*		spincrfld_;
};



// uiManageLineGeomDlg
uiManageLineGeomDlg::uiManageLineGeomDlg( uiParent* p, Pos::GeomID geomid,
					  bool readonly )
    : uiDialog(p,uiDialog::Setup(tr("Edit Line Geometry"),mNoDlgTitle,
				  mODHelpKey(mManageLineGeomDlgHelpID)))
    , geomid_(geomid)
    , readonly_(readonly)
{
    if ( readonly )
    {
	setCtrlStyle( CloseOnly );
	setCaption( tr("Browse Line Geometry") );
    }

    const BufferString linenm = Survey::GM().getName( geomid_ );
    uiString lbl = tr("%1 : %2").arg(uiStrings::sLineName()).arg(linenm);

    uiLabel* titllbl = new uiLabel( this, lbl );
    titllbl->attach( hCentered );

    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(geomid_));
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
    collbls.add( uiStrings::sTraceNumber() ).add( uiStrings::sSPNumber() )
	   .add( uiStrings::sX() ).add( uiStrings::sY() );
    table_->setColumnLabels( collbls );
    if ( readonly )
	table_->setTableReadOnly( true );

    FloatInpIntervalSpec spec( true );
    uiString zlbl = uiStrings::sZRange();
    zlbl.withSurvZUnit();
    rgfld_ = new uiGenInput( this, zlbl, spec );
    rgfld_->attach( leftAlignedBelow, table_ );
    StepInterval<float> zrg = geom2d->data().zRange();
    zrg.scale( mCast(float,SI().zDomain().userFactor()) );
    rgfld_->setValue( zrg );
    rgfld_->setReadOnly( readonly );

    if ( !readonly )
    {
	uiButtonGroup* grp =
		new uiButtonGroup( this, "buttons", OD::Horizontal );
	new uiPushButton( grp, tr("Set new Geometry"),
			  mCB(this,uiManageLineGeomDlg,impGeomCB), false );
	new uiPushButton( grp, tr("Set Trace/SP Number"),
			  mCB(this,uiManageLineGeomDlg,setTrcSPNrCB), false );
	grp->attach( centeredBelow, table_ );
	grp->attach( ensureBelow, rgfld_ );
    }

    fillTable( *geom2d );
}


uiManageLineGeomDlg::~uiManageLineGeomDlg()
{}


void uiManageLineGeomDlg::impGeomCB( CallBacker* )
{
    if ( readonly_ )
	return;

    const BufferString linenm = Survey::GM().getName( geomid_ );
    uiImp2DGeom dlg( this, linenm );
    if ( !dlg.go() ) return;

    RefMan<Survey::Geometry2D> geom = new Survey::Geometry2D( linenm );
    if ( !dlg.fillGeom(*geom) )
	return;

    table_->clearTable();
    fillTable( *geom );
}


void uiManageLineGeomDlg::setTrcSPNrCB( CallBacker* )
{
    if ( readonly_ )
	return;

    uiTrc2SPDlg dlg( this );
    if ( !dlg.go() )
	return;

    const bool calcsp = dlg.calcSP();
    const LinePars lp = dlg.getRelationship();
    const int fromidx = calcsp ? 0 : 1;
    const int toidx = calcsp ? 1 : 0;
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	const float var = table_->getFValue( RowCol(idx,fromidx) );
	const float val = lp.getValue( var );
	table_->setValue( RowCol(idx,toidx), val );
    }
}


void uiManageLineGeomDlg::fillTable( const Survey::Geometry2D& geom2d )
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


bool uiManageLineGeomDlg::acceptOK()
{
    if (!uiMSG().askGoOn(tr("Do you really want to change the geometry?\n"
			    "This will affect all associated data.")))
	return false;

    mDynamicCastGet(Survey::Geometry2D*,geom2d,
		    Survey::GMAdmin().getGeometry(geomid_) )
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

    StepInterval<float> newzrg = rgfld_->getFStepInterval();
    if ( newzrg.isUdf() )
    {
	uiMSG().error( tr("Please set valid Z range") );
	return false;
    }

    newzrg.scale( 1.f/mCast(float,SI().zDomain().userFactor()) );
    geom2d->dataAdmin().setZRange( newzrg );
    geom2d->touch();

    uiString errmsg;
    if ( !Survey::GMAdmin().write(*geom2d,errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    return true;
}

