/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Sep 2001
 RCS:		$Id: uisegyexp.cc,v 1.5 2008-10-08 15:57:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyexp.h"
#include "uisegydef.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "segyhdr.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uitaskrunner.h"
#include "uifileinput.h"
#include "executor.h"
#include "ctxtioobj.h"
#include "iostrm.h"
#include "ioman.h"
#include "filepath.h"
#include "filegen.h"
#include "seistrctr.h"


uiSEGYExp::uiSEGYExp( uiParent* p, Seis::GeomType gt )
	: uiDialog(p,uiDialog::Setup("SEG-Y I/O","Export to SEG-Y","103.0.7"))
	, ctio_(*mMkCtxtIOObj(SeisTrc))
    	, geom_(gt)
    	, morebut_(0)
{
    seissel_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(geom_) );
    seissel_->selectiondone.notify( mCB(this,uiSEGYExp,inpSel) );

    transffld_ = new uiSeisTransfer( this, uiSeisTransfer::Setup(geom_)
				    .withnullfill(true)
				    .fornewentry(false) );
    transffld_->attach( alignedBelow, seissel_ );

    fpfld_ = new uiSEGYFilePars( this, false );
    fpfld_->attach( alignedBelow, transffld_ );

    uiSEGYFileSpec::Setup su; su.forread(false).canbe3d(!Seis::is2D(geom_));
    fsfld_ = new uiSEGYFileSpec( this, su );
    fsfld_->attach( alignedBelow, fpfld_ );

    if ( Seis::is2D(geom_) )
    {
	morebut_ = new uiCheckBox( this, "Export more from same Line Set" );
	morebut_->attach( alignedBelow, fsfld_ );
    }
}


uiSEGYExp::~uiSEGYExp()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiSEGYExp::inpSel( CallBacker* )
{
    if ( ctio_.ioobj )
	transffld_->updateFrom( *ctio_.ioobj );
}


class uiSEGYExpMore : public uiDialog
{
public:

uiSEGYExpMore( uiSEGYExp* p, const IOObj& ii, const IOObj& oi, const char* anm )
	: uiDialog(p,uiDialog::Setup("2D SEG-Y multi-export",
		    		     "Specify file details","103.0.7"))
	, inioobj_(ii)
	, outioobj_(oi)
	, segyexp_(p)
	, attrnm_(anm)
{
    setHelpID( "103.0.7" );
    const BufferString fnm( outioobj_.fullUserExpr(false) );
    FilePath fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Exp " ); setupnm += uiSEGYFileSpec::sKeyLineNmToken;

    uiLabel* lbl = 0;
    const bool isrealattrib = strcmp(attrnm_,LineKey::sKeyDefAttrib);
    if ( isrealattrib )
    {
	BufferString lbltxt( "Attribute: " );
	lbltxt += attrnm_;
	lbl = new uiLabel( this, lbltxt );
    }

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Lines to export",
						    true );
    lnmsfld_ = llb->box();
    SeisIOObjInfo sii( inioobj_ );
    BufferStringSet lnms;
    sii.getLineNamesWithAttrib( attrnm_, lnms );
    for ( int idx=0; idx<lnms.size(); idx++ )
	lnmsfld_->addItem( lnms.get(idx) );
    lnmsfld_->selectAll();
    if ( lbl )
	llb->attach( alignedBelow, lbl );

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken );
    if ( isrealattrib )
    {
	setupnm += " ("; setupnm += attrnm_; setupnm += ")";
	BufferString clnattrnm( attrnm_ );
	cleanupString( clnattrnm.buf(), mC_False, mC_False, mC_True );
	newfnm += "_"; newfnm += clnattrnm;
    }
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    BufferString txt( "Output (Line name replaces '" );
    txt += uiSEGYFileSpec::sKeyLineNmToken; txt += "')";

    fnmfld_ = new uiFileInput( this, txt,
		    uiFileInput::Setup(fp.fullPath()).forread(false) );
    fnmfld_->attach( alignedBelow, llb );
}


bool acceptOK( CallBacker* )
{
    BufferString fnm = fnmfld_->fileName();
    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    if ( !File_isDirectory(dirnm) )
	File_createDir( dirnm, 0 );
    if ( !File_isDirectory(dirnm) || !File_isWritable(dirnm) )
    {
	uiMSG().error( "Directory provided not usable" );
	return false;
    }
    if ( !strstr(fp.fullPath().buf(),uiSEGYFileSpec::sKeyLineNmToken) )
    {
	BufferString msg( "The file name has to contain at least one '" );
	msg += uiSEGYFileSpec::sKeyLineNmToken; msg += "'\n";
	msg += "That will then be replaced by the line name";
	uiMSG().error( msg );
	return false;
    }

    IOM().to( inioobj_.key() );
    return doExp( fp );
}


IOObj* getSubstIOObj( const char* fullfnm )
{
    IOObj* newioobj = outioobj_.clone();
    newioobj->setName( fullfnm );
    mDynamicCastGet(IOStream*,iostrm,newioobj)
    iostrm->setFileName( fullfnm );
    return newioobj;
}


bool doWork( IOObj* newioobj, const char* lnm, bool islast, bool& nofails )
{
    const IOObj& in = inioobj_; const IOObj& out = *newioobj;
    bool res = segyexp_->doWork( in, out, lnm, attrnm_ );
    delete newioobj;
    if ( !res )
    {
	nofails = false;
	if ( !islast && !uiMSG().askGoOn("Continue with next?") )
	    return false;
    }
    return true;
}

bool doExp( const FilePath& fp )
{
    BufferStringSet lnms;
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( lnmsfld_->isSelected(idx) )
	    lnms.add( lnmsfld_->textOfItem(idx) );
    }
    if ( lnms.size() < 1 )
    {
	uiMSG().error( "Please select lines to export" );
	return false;
    }

    bool nofails = true;
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const BufferString& lnm = *lnms[idx];
	BufferString filenm( fp.fullPath() );
	replaceString( filenm.buf(), uiSEGYFileSpec::sKeyLineNmToken, lnm );
	IOObj* newioobj = getSubstIOObj( filenm );
	if ( !doWork( newioobj, lnm, idx > lnms.size()-2, nofails ) )
	    return false;
    }

    return nofails;
}

    uiFileInput*	fnmfld_;
    uiListBox*		lnmsfld_;
    uiSEGYExp*		segyexp_;

    const IOObj&	inioobj_;
    const IOObj&	outioobj_;
    const char*		attrnm_;

};


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSEGYExp::acceptOK( CallBacker* )
{
    if ( !seissel_->commitInput(false) )
    {
	uiMSG().error( "Please select the data to export" );
	return false;
    }

    const IOObj* inioobj = ctio_.ioobj;
    PtrMan<IOObj> outioobj = fsfld_->getSpec().getIOObj( true );
    fpfld_->fillPar( outioobj->pars() );
    const bool is2d = Seis::is2D( geom_ );
    outioobj->pars().setYN( SeisTrcTranslator::sKeyIs2D, is2d );

    const char* attrnm = seissel_->attrNm();
    const char* lnm = is2d && transffld_->selFld2D()
			   && transffld_->selFld2D()->isSingLine()
		    ? transffld_->selFld2D()->selectedLine() : 0;
    if ( !morebut_ || !morebut_->isChecked() )
	return doWork( *inioobj, *outioobj, lnm, attrnm );
    else
    {
	uiSEGYExpMore dlg( this, *inioobj, *outioobj, attrnm );
	return dlg.go();
    }
}


bool uiSEGYExp::doWork( const IOObj& inioobj, const IOObj& outioobj,
				const char* linenm, const char* attrnm )
{
    const bool is2d = Seis::is2D( geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
    if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	return false;

    SEGY::TxtHeader::info2d = is2d;
    PtrMan<Executor> exec = transffld_->getTrcProc( inioobj, outioobj,
					"Export seismic data", "Putting traces",
					attrnm, linenm );
    if ( !exec )
	return false;

    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( exec->name() );
	nm += " ("; nm += linenm; nm += ")";
	exec->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = dlg.execute( *exec );
    exec.erase();

    SEGY::TxtHeader::info2d = false;
    return rv;
}
