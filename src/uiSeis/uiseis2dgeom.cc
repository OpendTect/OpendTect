/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseis2dgeom.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "posinfo2dsurv.h"
#include "od_iostream.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "od_helpids.h"

static const BufferStringSet emptylnms;


uiSeisDump2DGeom::uiSeisDump2DGeom( uiParent* p, const IOObj* ioobj )
    : uiDialog(p,uiDialog::Setup(tr("Dump 2D line geometry to file"),
				 mNoDlgTitle,
				 mODHelpKey(mSeisDump2DGeomHelpID)))
{
    const CallBack cb( mCB(this,uiSeisDump2DGeom,seisSel) );
    if ( ioobj )
	preFinalise().notify( cb );

    uiSeisSel::Setup ss( Seis::Line );
    seisfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,true), ss );
    if ( ioobj )
	seisfld_->setInput( ioobj->key() );
    seisfld_->selectionDone.notify( cb );

    lnmsfld_ = new uiGenInput( this, tr("One line only"),
			      StringListInpSpec(emptylnms) );
    lnmsfld_->setWithCheck( true );
    lnmsfld_->attach( alignedBelow, seisfld_ );

    outfld_ = new uiASCIIFileInput( this, false );
    outfld_->attach( alignedBelow, lnmsfld_ );
}


static void getLineNames( const IOObj& ioobj, BufferStringSet& lnms )
{
    uiSeisIOObjInfo oinf( ioobj );
    oinf.ioObjInfo().getLineNames( lnms );
}


void uiSeisDump2DGeom::seisSel( CallBacker* )
{
    const IOObj* lsioobj = seisfld_->ioobj();
    if ( !lsioobj )
	return;

    BufferStringSet lnms;
    getLineNames( *lsioobj, lnms );
    lnmsfld_->newSpec( StringListInpSpec(lnms), 0 );
}


bool uiSeisDump2DGeom::acceptOK( CallBacker* )
{
    const IOObj* lsioobj = seisfld_->ioobj();
    if ( !lsioobj )
        return false;

    BufferString fnm( outfld_->fileName() );
    if ( fnm.isEmpty() )
    {
        uiMSG().error( tr("Please enter the output file name") );
        return false;
    }

    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
        uiMSG().error( tr("Cannot open the output file") );
        return false;
    }

    BufferString lsnm( lsioobj->name() );
    S2DPOS().setCurLineSet( lsnm );

    BufferStringSet lnms;
    if ( lnmsfld_->isChecked() )
	lnms.add( lnmsfld_->text() );
    else
	getLineNames( *lsioobj, lnms );

    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	PosInfo::Line2DData l2dd( lnms.get(idx) );
	S2DPOS().getGeometry( l2dd );
	l2dd.dump( strm, true );
	strm << "\n\n";
    }

    return true;
}
