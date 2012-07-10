/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uisurfaceman.cc,v 1.101 2012-07-10 08:05:35 cvskris Exp $";


#include "uisurfaceman.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "oddirs.h"
#include "strmprov.h"
#include "survinfo.h"

#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "embodytr.h"
#include "emmarchingcubessurface.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"

#include "uibodyoperatordlg.h"
#include "uibodyregiondlg.h"
#include "uiimpbodycaldlg.h"
#include "uigeninputdlg.h"
#include "uihorizonmergedlg.h"
#include "uihorizonrelations.h"
#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uiiosurfacedlg.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"


#define mGet( typ, hor2d, hor3d, anyhor, emfss, flt3d,  body ) \
    !strcmp(typ,EMBodyTranslatorGroup::sKeyword()) ? body : \
    (!strcmp(typ,EMHorizon2DTranslatorGroup::keyword()) ? hor2d : \
    (!strcmp(typ,EMHorizon3DTranslatorGroup::keyword()) ? hor3d : \
    (!strcmp(typ,EMAnyHorizonTranslatorGroup::keyword()) ? anyhor : \
    (!strcmp(typ,EMFaultStickSetTranslatorGroup::keyword()) ? emfss : flt3d))))

#define mGetIoContext(typ) \
    mGet( typ, EMHorizon2DTranslatorGroup::ioContext(), \
	       EMHorizon3DTranslatorGroup::ioContext(), \
	       EMAnyHorizonTranslatorGroup::ioContext(), \
	       EMFaultStickSetTranslatorGroup::ioContext(), \
	       EMFault3DTranslatorGroup::ioContext(), \
	       EMBodyTranslatorGroup::ioContext() )

#define mGetManageStr(typ) \
    mGet( typ, "Manage 2D Horizons", "Manage 3D Horizons", "Manage Horizons", \
	       "Manage FaultStickSets", "Manage Faults", "Manage Bodies" )

#define mGetCopyStr(typ) \
    mGet( typ, "Copy 2D horizon", "Copy 3D horizon", "Copy horizon", \
	       "Copy faultStickSet", "Copy fault","Copy body" )

#define mGetHelpID(typ) \
    mGet( typ, "104.2.1", "104.2.0", "104.2.0", "104.2.4", "104.2.5", "104.2.6")

using namespace EM;

mDefineInstanceCreatedNotifierAccess(uiSurfaceMan)


uiSurfaceMan::uiSurfaceMan( uiParent* p, const char* typ )
    : uiObjFileMan(p,uiDialog::Setup(mGetManageStr(typ),mNoDlgTitle,
                                     mGetHelpID(typ)).nrstatusflds(1),
		   mGetIoContext(typ) )
    , attribfld_(0)
    , man2dbut_(0)
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();

    if ( strcmp(typ,EMBodyTranslatorGroup::sKeyword()) )
    	manipgrp->addButton( "copyobj", mGetCopyStr(typ),
		mCB(this,uiSurfaceMan,copyCB) );

    if ( mGet(typ,true,false,true,false,false,false) )
    {
	man2dbut_ = manipgrp->addButton( "man2d", "Manage 2D Horizons",
					 mCB(this,uiSurfaceMan,man2dCB) );
	man2dbut_->setSensitive( false );
    }

    if ( mGet(typ,false,true,false,false,false,false) )
	manipgrp->addButton( "mergehorizons", "Merge 3D Horizons",
			     mCB(this,uiSurfaceMan,merge3dCB) );

    if ( mGet(typ,false,true,true,false,false,false) )
    {
	uiLabeledListBox* llb = new uiLabeledListBox( listgrp_,
		"Horizon Data", true, uiLabeledListBox::AboveLeft );
	llb->attach( rightOf, selgrp_ );
	attribfld_ = llb->box();
	attribfld_->setToolTip( "Horizon Data (Attributes stored in Horizon format)" );

	uiManipButGrp* butgrp = new uiManipButGrp( llb );
	butgrp->addButton( uiManipButGrp::Remove,"Remove selected Horizon Data",
			   mCB(this,uiSurfaceMan,removeAttribCB) );
	butgrp->addButton( uiManipButGrp::Rename, "Rename selected Horizon Data",
			   mCB(this,uiSurfaceMan,renameAttribCB) );
	butgrp->attach( rightTo, attribfld_ );

	uiPushButton* stratbut =
	    new uiPushButton( listgrp_, "&Stratigraphy", false );
	stratbut->activated.notify( mCB(this,uiSurfaceMan,stratSel) );
	stratbut->attach( alignedBelow, selgrp_ );

	uiPushButton* relbut = new uiPushButton( listgrp_, "&Relations", false);
	relbut->activated.notify( mCB(this,uiSurfaceMan,setRelations) );
	relbut->attach( rightTo, stratbut );
	relbut->attach( ensureBelow, llb );

	setPrefWidth( 50 );
    }

    if ( mGet(typ,false,false,false,false,false,true) )
    {
	manipgrp->addButton( "set_union", "Merge bodies",
		mCB(this,uiSurfaceMan,mergeBodyCB) );
	manipgrp->addButton( "set_implicit", "Create region body",
		mCB(this,uiSurfaceMan,createBodyRegionCB) );
	manipgrp->addButton( "bodyvolume", "Volume estimate",
		mCB(this,uiSurfaceMan,calVolCB) );
    }

    mTriggerInstanceCreatedNotifier();
    selChg( this ); 
}


uiSurfaceMan::~uiSurfaceMan()
{}


void uiSurfaceMan::addTool( uiButton* but )
{
    uiObjFileMan::addTool( but );
    if ( !lastexternal_ && attribfld_ )
	but->attach( alignedBelow, attribfld_ );
}


const char* uiSurfaceMan::getDefKey() const
{
    if ( !curioobj_ )
	return uiObjFileMan::getDefKey();

    return IOPar::compKey( sKey::Default(), curioobj_->group() );
}


bool uiSurfaceMan::isCur2D() const
{
    return curioobj_ && 
	   !strcmp(curioobj_->group(),EMHorizon2DTranslatorGroup::keyword());
}


bool uiSurfaceMan::isCurFault() const
{
    BufferString grp = curioobj_ ? curioobj_->group() : "";
    return grp==EMFaultStickSetTranslatorGroup::keyword() ||
	   grp==EMFault3DTranslatorGroup::keyword();
}


void uiSurfaceMan::copyCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    PtrMan<IOObj> ioobj = curioobj_->clone();
    uiSurfaceRead::Setup su( ioobj->group() );
    su.withattribfld(true).withsubsel(!isCurFault()).multisubsel(true);
    uiCopySurface dlg( this, *ioobj, su );
    if ( dlg.go() )
	selgrp_->fullUpdate( ioobj->key() );
}


void uiSurfaceMan::man2dCB( CallBacker* )
{
    EM::IOObjInfo eminfo( curioobj_->key() );
    uiSurface2DMan dlg( this, eminfo );
    dlg.go();
}


void uiSurfaceMan::merge3dCB( CallBacker* )
{
    uiHorizonMergeDlg dlg( this, false );
    dlg.go();
}


void uiSurfaceMan::mergeBodyCB( CallBacker* )
{
    uiBodyOperatorDlg dlg( this );
    if ( dlg.go() )
    	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::calVolCB( CallBacker* )
{
    RefMan<EM::EMObject> emo = 
	EM::EMM().loadIfNotFullyLoaded( curioobj_->key(), 0 );
    mDynamicCastGet( EM::Body*, emb, emo.ptr() );
    if ( !emb ) 
    {
	uiMSG().error( "Body is empty" );
	return;
    }

    uiImplBodyCalDlg dlg( this, *emb );
    dlg.go();
}


void uiSurfaceMan::createBodyRegionCB( CallBacker* )
{
    uiBodyRegionDlg dlg( this );
    if ( dlg.go() )
    	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::setRelations( CallBacker* )
{
    uiHorizonRelationsDlg dlg( this, isCur2D() );
    dlg.go();
}


void uiSurfaceMan::removeAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    if ( curioobj_->implReadOnly() )
    {
	uiMSG().error( "Could not remove Horizon Data. Surface is read-only" );
	return;
    }

    BufferStringSet attrnms;
    attribfld_->getSelectedItems( attrnms );
    if ( attrnms.isEmpty() || 
	    !uiMSG().askRemove("All selected Horizon Data will be removed.\n"
			       "Do you want to continue?") )
	return;

    for ( int ida=0; ida<attrnms.size(); ida++ )
	SurfaceAuxData::removeFile( *curioobj_, attrnms.get(ida) );

    selChg( this );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiSurfaceMan::renameAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const char* attribnm = attribfld_->getText();
    BufferString titl( "Rename '" ); titl += attribnm; titl += "'";
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(attribnm) );
    if ( !dlg.go() ) return;

    const char* newnm = dlg.text();
    if ( attribfld_->isPresent(newnm) )
	mErrRet( "Name is already in use" )

    const BufferString filename =
		SurfaceAuxData::getFileName( *curioobj_, attribnm );
    if ( File::isEmpty(filename) )
	mErrRet( "Cannot find Horizon Data file" )
    else if ( !File::isWritable(filename) )
	mErrRet( "The Horizon Data file is not writable" )

    StreamData sdin( StreamProvider(filename).makeIStream() );
    if ( !sdin.usable() )
	mErrRet( "Cannot open Horizon Data file for read" )
    BufferString ofilename( filename ); ofilename += "_new";
    StreamData sdout( StreamProvider(ofilename).makeOStream() );
    if ( !sdout.usable() )
    {
	sdin.close();
	mErrRet( "Cannot open new Horizon Data file for write" )
    }

    ascistream aistrm( *sdin.istrm );
    ascostream aostrm( *sdout.ostrm );
    aostrm.putHeader( aistrm.fileType() );
    IOPar iop( aistrm );
    iop.set( sKey::Attribute(), newnm );
    iop.putTo( aostrm );

    char c;
    while ( *sdin.istrm )
	{ sdin.istrm->read( &c, 1 ); sdout.ostrm->write( &c, 1 ); }
    const bool writeok = sdout.ostrm->good();
    sdin.close(); sdout.close();
    BufferString tmpfnm( filename ); tmpfnm += "_old";
    if ( !writeok )
    {
	File::remove( ofilename );
	mErrRet( "Error during write. Reverting to old name" )
    }

    if ( File::rename(filename,tmpfnm) )
	File::rename(ofilename,filename);
    else
    {
	File::remove( ofilename );
	mErrRet( "Cannot rename file(s). Reverting to old name" )
    }

    if ( File::exists(tmpfnm) )
	File::remove( tmpfnm );

    selChg( this );
}


void uiSurfaceMan::fillAttribList( const BufferStringSet& strs )
{
    if ( !attribfld_ ) return;

    attribfld_->setEmpty();
    for ( int idx=0; idx<strs.size(); idx++)
	attribfld_->addItem( strs[idx]->buf() );
    attribfld_->selectAll( false );
}


void uiSurfaceMan::mkFileInfo()
{
#define mAddRangeTxt(inl) \
    range = inl ? eminfo.getInlRange() : eminfo.getCrlRange(); \
    if ( range.isUdf() ) \
	txt += "-\n"; \
    else \
    { \
	txt += range.start; txt += " - "; txt += range.stop; \
	txt += " - "; txt += range.step; txt += "\n"; \
    }

    BufferString txt;
    EM::IOObjInfo eminfo( curioobj_ );
    if ( !eminfo.isOK() )
    {
	txt += eminfo.name(); txt.add( " has no file on disk (yet).\n" );
	setInfo( txt );
	return;
    }

    BufferStringSet attrnms;
    if ( eminfo.getAttribNames(attrnms) )
	fillAttribList( attrnms );

    if ( man2dbut_ )
	man2dbut_->setSensitive( isCur2D() );

    if ( isCur2D() || isCurFault() )
    {
	txt = isCur2D() ? "Nr. 2D lines: " : "Nr. Sticks: "; 
	if ( isCurFault() )
	    txt += eminfo.nrSticks();
	else
	{
	    BufferStringSet linenames;
	    if ( eminfo.getLineNames(linenames) )
		txt += linenames.size();
	    else
		txt += "-";
	}

	txt += "\n";
    }
    else
    {
	StepInterval<int> range;
	txt = "Inline range: "; mAddRangeTxt(true)
	txt += "Crossline range: "; mAddRangeTxt(false)
	Interval<float> zrange = eminfo.getZRange();
	if ( !zrange.isUdf() )
	{
	    txt += "Z range"; txt += SI().getZUnitString(); txt += ": ";
	    txt += mNINT32( zrange.start * SI().zDomain().userFactor() );
	    txt += " - ";
	    txt += mNINT32( zrange.stop * SI().zDomain().userFactor() );
	    txt += "\n";
	}
    }

    txt += getFileInfo();

    BufferStringSet sectionnms;
    eminfo.getSectionNames( sectionnms );
    if ( sectionnms.size() > 1 )
    {
	txt += "Nr of sections: "; txt += sectionnms.size(); txt += "\n";
	for ( int idx=0; idx<sectionnms.size(); idx++ )
	{
	    txt += "\tPatch "; txt += idx+1; txt += ": "; 
	    txt += sectionnms[idx]->buf(); txt += "\n";
	}
    }

    setInfo( txt );
}


double uiSurfaceMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( File::isEmpty(filenm) ) return -1;
    double totalsz = (double)File::getKbSize( filenm );
    nrfiles = 1;

    const BufferString basefnm( filenm );
    for ( int idx=0; ; idx++ )
    {
	BufferString fnm( basefnm ); fnm += "^"; fnm += idx; fnm += ".hov";
	if ( !File::exists(fnm) ) break;
	totalsz += (double)File::getKbSize( fnm );
	nrfiles++;
    }

    return totalsz;
}


class uiSurfaceStratDlg : public uiDialog
{
public:
uiSurfaceStratDlg( uiParent* p,  const ObjectSet<MultiID>& ids )
    : uiDialog(p,uiDialog::Setup("Stratigraphy",mNoDlgTitle,""))
    , objids_(ids)
{
    tbl_ = new uiTable( this, uiTable::Setup(ids.size(),3), 
			"Stratigraphy Table" );
    BufferStringSet lbls; lbls.add( "Name" ).add( "Color" ).add( "Marker" );
    tbl_->setColumnLabels( lbls );
    tbl_->setTableReadOnly( true );
    tbl_->setRowResizeMode( uiTable::Interactive );
    tbl_->setColumnResizeMode( uiTable::ResizeToContents );
    tbl_->setColumnStretchable( 2, true );
    tbl_->setPrefWidth( 400 );

    uiToolButton* sb = new uiToolButton( this, "man_strat",
	    				"Edit Stratigraphy to define Markers",
					mCB(this,uiSurfaceStratDlg,doStrat) );
    sb->attach( rightOf, tbl_ );

    IOPar par;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	par.setEmpty();
	EM::EMM().readPars( *ids[idx], par );
	tbl_->setText( RowCol(idx,0), EM::EMM().objectName(*ids[idx]) );
	Color col( Color::White() );
	par.get( sKey::Color(), col );
	tbl_->setColor( RowCol(idx,1), col );

	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, true, 0 );
	levelsel->selChange.notify( mCB(this,uiSurfaceStratDlg,lvlChg) );
	tbl_->setCellGroup( RowCol(idx,2), levelsel );
	int lvlid = -1;
	par.get( sKey::StratRef(), lvlid );
	levelsel->setID( lvlid );
    }
}


protected:

void doStrat( CallBacker* )
{
    StratTWin().popUp();
}

void lvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb) 
    if ( !levelsel ) return;

    const Color col = levelsel->getColor();
    if ( col == Color::NoColor() ) return;

    const RowCol rc = tbl_->getCell( levelsel );
    tbl_->setColor( RowCol(rc.row,1), col );
}

bool acceptOK( CallBacker* )
{
    for ( int idx=0; idx<objids_.size(); idx++ )
    {
	IOPar par;
	Color col = tbl_->getColor( RowCol(idx,1) );
	par.set( sKey::Color(), col );

	mDynamicCastGet(uiStratLevelSel*,levelsel,
			tbl_->getCellGroup(RowCol(idx,2)))
	const int lvlid = levelsel ? levelsel->getID() : -1;
	par.set( sKey::StratRef(), lvlid );
	EM::EMM().writePars( *objids_[idx], par );
    }

    return true;
}


    uiTable*	tbl_;
    const ObjectSet<MultiID>& objids_;

};


void uiSurfaceMan::stratSel( CallBacker* )
{
    const ObjectSet<MultiID>& ids = selgrp_->getIOObjIds();
    uiSurfaceStratDlg dlg( this, ids );
    dlg.go();
}


uiSurface2DMan::uiSurface2DMan( uiParent* p, const IOObjInfo& info )
    :uiDialog(p,uiDialog::Setup("2D Horizons management","Manage 2D horizons",
				"104.2.1"))
    , eminfo_(info)
{
    setCtrlStyle( LeaveOnly );

    uiGroup* topgrp = new uiGroup( this, "Top" );
    uiLabeledListBox* lllb = new uiLabeledListBox( topgrp, "2D lines", false,
						   uiLabeledListBox::AboveMid );
    linelist_ = lllb->box();
    BufferStringSet linenames;
    info.getLineNames( linenames );
    linelist_->addItems( linenames );
    linelist_->selectionChanged.notify( mCB(this,uiSurface2DMan,lineSel) );

    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    lineSel( 0 );
}


void uiSurface2DMan::lineSel( CallBacker* )
{
    const int curitm = linelist_->currentItem();
    BufferStringSet linesets;
    eminfo_.getLineSets( linesets );
    if ( !linesets.validIdx(curitm) )
    {
	infofld_->setText( "" );
	return;
    }

    BufferString txt( "LineSet name: ", linesets.get(curitm), "\n" );
    TypeSet< StepInterval<int> > trcranges;
    eminfo_.getTrcRanges( trcranges );

    if ( trcranges.validIdx(curitm) )
    {
	StepInterval<int> trcrg = trcranges[ curitm ];
	txt += BufferString( sKey::FirstTrc(), ": " ); txt += trcrg.start;
	txt += "\n";
	txt += BufferString( sKey::LastTrc(), ": " ); txt += trcrg.stop;
	txt += "\n";
	txt += BufferString( "Trace Step: " ); txt += trcrg.step;
    }

    infofld_->setText( txt );
}

