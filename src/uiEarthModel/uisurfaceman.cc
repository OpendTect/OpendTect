/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uisurfaceman.cc,v 1.41 2007-11-29 14:36:04 cvsbert Exp $
________________________________________________________________________

-*/


#include "uisurfaceman.h"

#include "ctxtioobj.h"
#include "filegen.h"
#include "ioobj.h"
#include "oddirs.h"
#include "pixmap.h"
#include "strmprov.h"
#include "ascstream.h"

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
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();

    manipgrp->addButton( ioPixmap("copyobj.png"), mCB(this,uiSurfaceMan,copyCB),
			 mGetCopyStr(typ) );

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

    uiPushButton* relbut = new uiPushButton( this, "&Relations", false );
    relbut->activated.notify( mCB(this,uiSurfaceMan,setRelations) );
    relbut->attach( alignedBelow, selgrp );
    relbut->attach( ensureBelow, attribfld );

    infofld->attach( ensureBelow, relbut );
    selChg( this ); 
}


uiSurfaceMan::~uiSurfaceMan()
{}


bool uiSurfaceMan::isCur2D() const
{
    return curioobj_ && 
	   !strcmp(curioobj_->group(),EMHorizon2DTranslatorGroup::keyword);
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
	if ( isCur2D() )
	{
	    txt = "Nr. 2D lines: "; 
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
