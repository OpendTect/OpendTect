/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          June 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiexport2dhorizon.cc,v 1.11 2010-09-29 02:25:54 cvsnanne Exp $";

#include "uiexport2dhorizon.h"

#include "ctxtioobj.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "keystrs.h"
#include "uifileinput.h"
#include "uicombobox.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include <stdio.h>


uiExport2DHorizon::uiExport2DHorizon( uiParent* p,
       				      const ObjectSet<SurfaceInfo>& hinfos )
	: uiDialog(p,uiDialog::Setup("Export 2D Horizon",
				     "Specify output parameters","104.0.1"))
	, hinfos_(hinfos)
{
    setCtrlStyle( DoAndStay );

    uiLabeledComboBox* lcbox = new uiLabeledComboBox( this, "Select Horizon",
	   					      "Select 2D Horizon" );
    horselfld_ = lcbox->box();
    horselfld_->selectionChanged.notify( mCB(this,uiExport2DHorizon,horChg) );
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	horselfld_->addItem( hinfos_[idx]->name );

    uiLabeledListBox* llbox = new uiLabeledListBox( this, "Select lines",
	    					    true );
    llbox->attach( alignedBelow, lcbox );
    linenmfld_ = llbox->box();

    udffld_ = new uiGenInput( this, "Write undefined parts? Undef value",
	    		     StringInpSpec(sKey::FloatUdf) );
    udffld_->setChecked( true );
    udffld_->setWithCheck( true );
    udffld_->attach( alignedBelow, llbox );

    wrlnmsbox_ = new uiCheckBox( this, "Write line name" );
    wrlnmsbox_->attach( alignedBelow, udffld_ );
    wrlnmsbox_->setChecked( true );
    bool setchk = true;
    if ( SI().zIsTime() )
	zbox_ = new uiCheckBox( this, "Z in msec" );
    else
    {
	zbox_ = new uiCheckBox( this, "Z in feet" );
	setchk = SI().depthsInFeetByDefault();
    }
    zbox_->setChecked( setchk );
    zbox_->attach( rightTo, wrlnmsbox_ );

    outfld_ = new uiFileInput( this, "Output Ascii file",
	    		      uiFileInput::Setup().forread(false) );
    outfld_->attach( alignedBelow, wrlnmsbox_ );

    horChg( 0 );
}


uiExport2DHorizon::~uiExport2DHorizon()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExport2DHorizon::doExport()
{
    BufferStringSet linenms;
    linenmfld_->getSelectedItems( linenms );
    if ( !linenms.size() )
	mErrRet("Please select at least one line to proceed")

    BufferString filenm = outfld_->fileName();
    StreamData sd = StreamProvider( filenm ).makeOStream();
    if ( !sd.usable() )
	mErrRet("Cannot write to output file")

    const int horidx = horselfld_->currentItem();
    if ( horidx < 0 || horidx > hinfos_.size() )
	mErrRet("Invalid Horizon")

    MultiID horid = hinfos_[horidx]->multiid;
    EM::EMManager& em = EM::EMM();
    EM::EMObject* obj = em.getObject( em.getObjectID(horid) );
    if ( !obj )
    {
	PtrMan<Executor> exec = em.objectLoader( horid );
	if ( !exec || !exec->execute() )
	    mErrRet("Cannot load horizon")

	obj = em.getObject( em.getObjectID(horid) );
	if ( !obj ) return false;

	obj->ref();
    }

    mDynamicCastGet(EM::Horizon2D*,hor,obj);
    if ( !hor )
	mErrRet("Cannot load horizon")
    
    EM::SectionID sid = hor->sectionID( 0 );
    const Geometry::Horizon2DLine* geom = hor->geometry().sectionGeometry(sid);
    if ( !geom ) mErrRet("Error Reading Horizon")

    const bool wrudfs = udffld_->isChecked();
    BufferString undefstr;
    if ( wrudfs )
    {
	undefstr =  udffld_->text();
	if ( undefstr.isEmpty() )
	    undefstr = "-";
    }

    const float zfac = !zbox_->isChecked() ? 1
		     : (SI().zIsTime() ? 1000 : mToFeetFactor);
    const bool wrlnms = wrlnmsbox_->isChecked();
    char buf[180];
    for ( int idx=0; idx< linenms.size(); idx++ )
    {
	BufferString linename = linenms.get( idx );
	const int lineidx = hor->geometry().lineIndex( linename );
	const int lineid = hor->geometry().lineID( lineidx );
	StepInterval<int> trcrg = geom->colRange( lineid );
	for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
	{
	    Coord3 pos = geom->getKnot( RowCol(lineid,trcnr) );

	    if ( mIsUdf(pos.x) || mIsUdf(pos.y) )
		continue;
	    const bool zudf = mIsUdf(pos.z);
	    if ( zudf && !wrudfs )
		continue;
	   
	    const bool hasspace = strchr( linename.buf(), ' ' ) ||
				  strchr( linename.buf(), '\t' );
	    BufferString controlstr = hasspace ? "\"%15s\"" : "%15s";

	    if ( zudf )
	    {
		if ( wrlnms )
		{
		    controlstr += "%16.2lf%16.2lf%8d%16s"; 
		    sprintf( buf, controlstr.buf(), linename.buf(),
			     pos.x, pos.y, trcnr, undefstr.buf() );
		}
		else
		{
		    BufferString out;
		    out += pos.x; out += "\t"; out += pos.y; out += "\t";
		    out += undefstr;
		    strcpy( buf, out.buf() );
		}
	    }
	    else
	    {
		pos.z *= zfac;
		if ( wrlnms )
		{
		    controlstr += "%16.2lf%16.2lf%8d%16.2lf";
		    sprintf( buf, controlstr.buf(),
			    linename.buf(), pos.x, pos.y, trcnr, pos.z );
		}
		else
		{
		    BufferString out;
		    out += pos.x; out += "\t"; out += pos.y; out += "\t";
		    out += pos.z;
		    strcpy( buf, out.buf() );
		}
	    }

	    *sd.ostrm << buf << '\n';
	    if ( sd.ostrm->bad() )
		mErrRet("Error writing to the output file")
	}
    }

    sd.close();
    return true;
}


bool uiExport2DHorizon::acceptOK( CallBacker* )
{
    if ( !strcmp(outfld_->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File::exists(outfld_->fileName()) && 
		!uiMSG().askOverwrite("Output file exists. Overwrite?") )
	return false;

    const bool res = doExport();
    if ( res )
	uiMSG().message( "Horizon successfully exported" );
    return false;
}


void uiExport2DHorizon::horChg( CallBacker* cb )
{
    linenmfld_->empty();
    const int horidx = horselfld_->currentItem();
    if ( horidx < 0 || horidx > hinfos_.size() )
	return;

    MultiID horid = hinfos_[horidx]->multiid;
    EM::EMManager& em = EM::EMM();

    PtrMan<IOObj> ioobj = IOM().get( horid );
    if ( !ioobj ) return;

    PtrMan<IOPar> pars = em.getSurfacePars( *ioobj );
    if ( !pars ) return;

    BufferStringSet linenames;
    pars->get( "Line names", linenames );
    linenmfld_->addItems( linenames );
}
