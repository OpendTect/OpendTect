/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uisurfaceman.cc,v 1.47 2008-05-26 12:15:57 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uisurfaceman.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "ioobj.h"
#include "multiid.h"
#include "oddirs.h"
#include "pixmap.h"
#include "strmprov.h"

#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"

#include "uibutton.h"
#include "uigeninputdlg.h"
#include "uihorizonrelations.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uiiosurfacedlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistratlvlsel.h"
#include "uitable.h"
#include "uitextedit.h"


#define mGet( typ, hor2d, hor3d, anyhor, flt ) \
    !strcmp(typ,EMHorizon2DTranslatorGroup::keyword) ? hor2d : \
    (!strcmp(typ,EMHorizon3DTranslatorGroup::keyword) ? hor3d : \
    (!strcmp(typ,EMAnyHorizonTranslatorGroup::keyword) ? anyhor : flt) )

#define mGetIoContext(typ) \
    mGet( typ, EMHorizon2DTranslatorGroup::ioContext(), \
	       EMHorizon3DTranslatorGroup::ioContext(), \
	       EMAnyHorizonTranslatorGroup::ioContext(), \
	       EMFaultTranslatorGroup::ioContext() )

#define mGetManageStr(typ) \
    mGet( typ, "Manage 2D horizons", "Manage 3D horizons", "Manage horizons", \
	       "Manage faults" )

#define mGetCopyStr(typ) \
    mGet( typ, "Copy 2D horizon", "Copy 3D horizon", "Copy horizon", \
	       "Copy fault" )

using namespace EM;

uiSurfaceMan::uiSurfaceMan( uiParent* p, const char* typ )
    : uiObjFileMan(p,uiDialog::Setup("Surface file management",
                                     mGetManageStr(typ),
                                     "104.2.0").nrstatusflds(1),
		   mGetIoContext(typ) )
    , attribfld(0)
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();

    manipgrp->addButton( ioPixmap("copyobj.png"), mCB(this,uiSurfaceMan,copyCB),
			 mGetCopyStr(typ) );

    if ( mGet(typ,true,true,true,false) )
    {
	attribfld = new uiListBox( this, "Calculated attributes", true );
	attribfld->attach( rightOf, selgrp );
	attribfld->setToolTip( "Calculated attributes" );

	uiManipButGrp* butgrp = new uiManipButGrp( this );
	butgrp->addButton( uiManipButGrp::Remove,
			   mCB(this,uiSurfaceMan,removeAttribCB),
			   "Remove selected attribute(s)" );
	butgrp->addButton( uiManipButGrp::Rename,
			   mCB(this,uiSurfaceMan,renameAttribCB),
			   "Rename selected attribute" );
	butgrp->attach( rightTo, attribfld );

	uiPushButton* stratbut =
	    new uiPushButton( this, "&Stratigraphy", false );
	stratbut->activated.notify( mCB(this,uiSurfaceMan,stratSel) );
	stratbut->attach( alignedBelow, selgrp );

	uiPushButton* relbut = new uiPushButton( this, "&Relations", false );
	relbut->activated.notify( mCB(this,uiSurfaceMan,setRelations) );
	relbut->attach( rightTo, stratbut );
	relbut->attach( ensureBelow, attribfld );
	infofld->attach( ensureBelow, relbut );
    }

    selChg( this ); 
}


uiSurfaceMan::~uiSurfaceMan()
{}


bool uiSurfaceMan::isCur2D() const
{
    return curioobj_ && 
	   !strcmp(curioobj_->group(),EMHorizon2DTranslatorGroup::keyword);
}


bool uiSurfaceMan::isCurFault() const
{
    return curioobj_ &&
	!strcmp(curioobj_->group(),EMFaultTranslatorGroup::keyword);
}


void uiSurfaceMan::copyCB( CallBacker* )
{
    if ( !curioobj_ ) return;
    PtrMan<IOObj> ioobj = curioobj_->clone();
    uiCopySurface dlg( this, *ioobj );
    if ( dlg.go() )
	selgrp->fullUpdate( ioobj->key() );
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
	uiMSG().error( "Could not remove attributes. Surface is read-only" );
	return;
    }

    BufferStringSet attrnms;
    attribfld->getSelectedItems( attrnms );
    if ( attrnms.isEmpty() || 
	    !uiMSG().askGoOn("All selected attributes will be removed.\n"
			     "Do you want to continue?") )
	return;

    for ( int ida=0; ida<attrnms.size(); ida++ )
    {
	const BufferString filenm = 
	    SurfaceAuxData::getAuxDataFileName( *curioobj_, attrnms.get(ida) );
	if ( filenm.isEmpty() ) continue;

	File_remove( filenm, NO );
    }

    selChg( this );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiSurfaceMan::renameAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const char* attribnm = attribfld->getText();
    BufferString titl( "Rename '" ); titl += attribnm; titl += "'";
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(attribnm) );
    if ( !dlg.go() ) return;

    const char* newnm = dlg.text();
    if ( attribfld->isPresent(newnm) )
	mErrRet( "Name is already in use" )

    const BufferString filename =
		SurfaceAuxData::getAuxDataFileName( *curioobj_, attribnm );
    if ( File_isEmpty(filename) )
	mErrRet( "Cannot find attribute file" )
    else if ( !File_isWritable(filename) )
	mErrRet( "The attribute data file is not writable" )

    StreamData sdin( StreamProvider(filename).makeIStream() );
    if ( !sdin.usable() )
	mErrRet( "Cannot open attribute file for read" )
    BufferString ofilename( filename ); ofilename += "_new";
    StreamData sdout( StreamProvider(ofilename).makeOStream() );
    if ( !sdout.usable() )
    {
	sdin.close();
	mErrRet( "Cannot open new attribute file for write" )
    }

    ascistream aistrm( *sdin.istrm );
    ascostream aostrm( *sdout.ostrm );
    aostrm.putHeader( aistrm.fileType() );
    IOPar iop( aistrm );
    iop.set( sKey::Attribute, newnm );
    iop.putTo( aostrm );

    char c;
    while ( *sdin.istrm )
	{ sdin.istrm->read( &c, 1 ); sdout.ostrm->write( &c, 1 ); }
    const bool writeok = sdout.ostrm->good();
    sdin.close(); sdout.close();
    BufferString tmpfnm( filename ); tmpfnm += "_old";
    if ( !writeok )
    {
	File_remove( ofilename, NO );
	mErrRet( "Error during write. Reverting to old name" )
    }

    if ( File_rename(filename,tmpfnm) )
	File_rename(ofilename,filename);
    else
    {
	File_remove( ofilename, NO );
	mErrRet( "Cannot rename file(s). Reverting to old name" )
    }

    if ( File_exists(tmpfnm) )
	File_remove( tmpfnm, NO );

    selChg( this );
}


void uiSurfaceMan::fillAttribList( const BufferStringSet& strs )
{
    if ( !attribfld ) return;

    attribfld->empty();
    for ( int idx=0; idx<strs.size(); idx++)
	attribfld->addItem( strs[idx]->buf() );
    attribfld->selectAll( false );
}


void uiSurfaceMan::mkFileInfo()
{
#define mRangeTxt(line) \
    txt += sd.rg.start.line; txt += " - "; txt += sd.rg.stop.line; \
    txt += " - "; txt += sd.rg.step.line; txt += "\n" \

    BufferString txt;
    SurfaceIOData sd;
    const char* res = EMM().getSurfaceData( curioobj_->key(), sd );
    if ( !res )
    {
	fillAttribList( sd.valnames );
	if ( isCur2D() || isCurFault() )
	{
	    txt = isCur2D() ? "Nr. 2D lines: " : "Nr. Sticks: "; 
	    txt += sd.rg.stop.inl - sd.rg.start.inl + 1; 
	    txt += "\n";
	}
	else
	{
	    txt = "Inline range: "; mRangeTxt(inl);
	    txt += "Crossline range: "; mRangeTxt(crl);
	}
    }

    txt += getFileInfo();

    if ( sd.sections.size() > 1 )
    {
	txt += "Nr of sections: "; txt += sd.sections.size(); txt += "\n";
	for ( int idx=0; idx<sd.sections.size(); idx++ )
	{
	    txt += "\tPatch "; txt += idx+1; txt += ": "; 
	    txt += sd.sections[idx]->buf(); txt += "\n";
	}
    }

    infofld->setText( txt );
}


double uiSurfaceMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( File_isEmpty(filenm) ) return -1;
    double totalsz = (double)File_getKbSize( filenm );
    nrfiles = 1;

    const BufferString basefnm( filenm );
    for ( int idx=0; ; idx++ )
    {
	BufferString fnm( basefnm ); fnm += "^"; fnm += idx; fnm += ".hov";
	if ( !File_exists(fnm) ) break;
	totalsz += (double)File_getKbSize( fnm );
	nrfiles++;
    }

    return totalsz;
}


class uiSurfaceStratDlg : public uiDialog
{
public:
uiSurfaceStratDlg( uiParent* p,  const ObjectSet<MultiID>& ids )
    : uiDialog(p,uiDialog::Setup("Stratigraphy","",""))
    , objids_(ids)
{
    tbl_ = new uiTable( this, uiTable::Setup(ids.size(),3), 
			"Stratigraphy Table" );
    BufferStringSet lbls; lbls.add( "Name" ).add( "Color" ).add( "Level" );
    tbl_->setColumnLabels( lbls );
    tbl_->setTableReadOnly( true );
    tbl_->setRowResizeMode( uiTable::Interactive );
    tbl_->setColumnResizeMode( uiTable::ResizeToContents );
    tbl_->setColumnStretchable( 2, true );
    tbl_->setPrefWidth( 400 );

    IOPar par;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	par.clear();
	EM::EMM().readPars( *ids[idx], par );
	tbl_->setText( RowCol(idx,0), EM::EMM().objectName(*ids[idx]) );
	Color col( Color::White );
	par.get( sKey::Color, col );
	tbl_->setColor( RowCol(idx,1), col );

	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, false, false );
	levelsel->levelChanged.notify( mCB(this,uiSurfaceStratDlg,lvlChg) );
	tbl_->setCellGroup( RowCol(idx,2), levelsel );
	int lvlid = -1;
	par.get( sKey::StratRef, lvlid );
	levelsel->setLevelID( lvlid );
    }
}


protected:

void lvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb) 
    const Color* col = levelsel ? levelsel->getLevelColor() : 0;
    if ( !col ) return;

    const RowCol rc = tbl_->getCell( levelsel );
    tbl_->setColor( RowCol(rc.row,1), *col );
}

bool acceptOK( CallBacker* )
{
    for ( int idx=0; idx<objids_.size(); idx++ )
    {
	IOPar par;
	Color col = tbl_->getColor( RowCol(idx,1) );
	par.set( sKey::Color, col );

	mDynamicCastGet(uiStratLevelSel*,levelsel,
			tbl_->getCellGroup(RowCol(idx,2)))
	const int lvlid = levelsel ? levelsel->getLevelID() : -1;
	par.set( sKey::StratRef, lvlid );
	EM::EMM().writePars( *objids_[idx], par );
    }

    return true;
}


    uiTable*	tbl_;
    const ObjectSet<MultiID>& objids_;

};


void uiSurfaceMan::stratSel( CallBacker* )
{
    const ObjectSet<MultiID>& ids = selgrp->getIOObjIds();
    uiSurfaceStratDlg dlg( this, ids );
    dlg.go();
}
