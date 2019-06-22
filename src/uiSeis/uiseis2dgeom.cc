/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/

#include "uiseis2dgeom.h"

#include "bufstringset.h"
#include "ioobjctxt.h"
#include "ioobj.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "posinfo2dsurv.h"
#include "od_iostream.h"

#include "uifilesel.h"
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

    uiFileSel::Setup fssu; fssu.setForWrite();
    outfld_ = new uiFileSel( this, uiStrings::sOutputFile(), fssu );
    outfld_->attach( alignedBelow, lnmsfld_ );
}


static void getLineNames( uiParent* p, const IOObj& ioobj,
			  BufferStringSet& lnms )
{
    uiSeisIOObjInfo oinf( p, ioobj );
    oinf.getLineNames( lnms );
}


void uiSeisDump2DGeom::seisSel( CallBacker* )
{
    const IOObj* lsioobj = seisfld_->ioobj();
    if ( !lsioobj )
	return;

    BufferStringSet lnms;
    getLineNames( this, *lsioobj, lnms );
    lnmsfld_->newSpec( StringListInpSpec(lnms), 0 );
}


bool uiSeisDump2DGeom::acceptOK()
{
    const IOObj* lsioobj = seisfld_->ioobj();
    if ( !lsioobj )
        return false;

    BufferString fnm( outfld_->fileName() );
    if ( fnm.isEmpty() )
    {
        uiMSG().error( uiStrings::phrEnter(tr(" the output file name")) );
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
	getLineNames( this, *lsioobj, lnms );

    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	PosInfo::Line2DData l2dd( lnms.get(idx) );
	S2DPOS().getGeometry( l2dd );
	l2dd.dump( strm, true );
	strm << "\n\n";
    }

    return true;
}
