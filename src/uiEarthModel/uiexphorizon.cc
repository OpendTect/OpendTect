/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.cc,v 1.4 2002-09-19 14:56:32 kristofer Exp $
________________________________________________________________________

-*/

#include "uiexphorizon.h"

#include "strmdata.h"
#include "strmprov.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "filegen.h"
#include "uimsg.h"
#include "uibinidsubsel.h"
#include "uilistbox.h"
#include "survinfo.h"
#include "geompos.h"
#include "surfaceinfo.h"
#include "ctxtioobj.h"


uiExportHorizon::uiExportHorizon( uiParent* p, 
				  const ObjectSet<SurfaceInfo>& hinfos )
	: uiDialog(p,uiDialog::Setup("Export Horizon",
				     "Specify output format",0))
	, hinfos_(hinfos)
	, selinfo_(-1)
{
    inbox = new uiLabeledListBox( this, "Available horizons" );
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	inbox->box()->addItem( hinfos_[idx]->name );
    inbox->box()->selectionChanged.notify( mCB(this,uiExportHorizon,selChg) );

    attrlbl = new uiLabel( this, "" );
    attrlbl->setHSzPol( uiObject::medium );
    attrlbl->attach( alignedBelow, inbox );
    uiLabel* lbl = new uiLabel( this, "Attached values: " );
    lbl->attach( leftOf, attrlbl );

    xyfld = new uiGenInput( this, "Positions in:",
                            BoolInpSpec("X/Y","Inl/Crl") );
    xyfld->attach( alignedBelow, attrlbl );

    BufferString lbltxt( "Include Z (" );
    lbltxt += SI().zIsTime() ? "Time)" : "Depth)";
    zfld = new uiGenInput( this, lbltxt, BoolInpSpec() );
    zfld->setValue( false );
    zfld->attach( alignedBelow, xyfld );

    outfld = new uiFileInput( this, "Output Ascii file", "", false );
    outfld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );
    outfld->attach( alignedBelow, zfld );

    selChg( 0 );
}


uiExportHorizon::~uiExportHorizon()
{
}


int uiExportHorizon::selHorID() const
{
    return selinfo_ < 0 ? -1 : hinfos_[selinfo_]->id;
}


#define mWarnRet(s) { uiMSG().warning(s); return false; }
#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportHorizon::writeAscii( const ObjectSet<
				  TypeSet<BinIDZValue> >& bizvset )
{
    const bool doxy = xyfld->getBoolValue();
    const bool addzpos = zfld->getBoolValue();

    const char* fname = outfld->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" );
    }

    for ( int iset=0; iset<bizvset.size(); iset++ )
    {
	const TypeSet<BinIDZValue>& bizvs = *bizvset[iset];

	for ( int idx=0; idx<bizvs.size(); idx++ )
	{
	    const BinIDZValue& bizv = bizvs[idx];
	    if ( doxy )
	    {
		Coord crd = SI().transform( bizv.binid );
		*sdo.ostrm << crd.x << '\t' << crd.y << '\t';
	    }
	    else
		*sdo.ostrm << bizv.binid.inl << '\t' << bizv.binid.crl << '\t';

	    if ( addzpos ) *sdo.ostrm << bizvs[idx].z << '\t';
	    *sdo.ostrm << bizvs[idx].value << '\n';
	}

	if ( iset < bizvset.size() - 1 )
	    // Should we write some kind of marker when more than 1 set?
	    ;
    }

    sdo.close();
    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( File_exists(outfld->fileName()) && 
			!uiMSG().askGoOn( "File exists. Continue?" ) )
	return false;

    if ( inbox->box()->currentItem() < 0 )
	mWarnRet( "Please select input" );

    return true;
}


const char* uiExportHorizon::selectedItem()
{
    return inbox->box()->getText();
}


void uiExportHorizon::selChg( CallBacker* )
{
    selinfo_ = inbox->box()->currentItem();
    attrlbl->setText( hinfos_[selinfo_]->attrnm );
}
