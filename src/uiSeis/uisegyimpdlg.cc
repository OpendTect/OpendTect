/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uisegyimpdlg.h"

#include "uisegydef.h"
#include "uiseistransf.h"
#include "uiseisfmtscale.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uiseparator.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uitoolbutton.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "segyhdr.h"
#include "seis2dline.h"
#include "seisioobjinfo.h"
#include "seisimporter.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "surv2dgeom.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "file.h"
#include "dirlist.h"
#include "ioman.h"
#include "iostrm.h"
#include "zdomain.h"



uiSEGYImpDlg::uiSEGYImpDlg( uiParent* p,
			const uiSEGYReadDlg::Setup& su, IOPar& iop )
    : uiSEGYReadDlg(p,su,iop)
    , morebut_(0)
    , ctio_(*uiSeisSel::mkCtxtIOObj(su.geom_,false))
{
    ctio_.ctxt.forread = false;
    if ( setup().dlgtitle_.isEmpty() )
    {
	BufferString ttl( "Import " );
	ttl += Seis::nameOf( setup_.geom_ );
	SEGY::FileSpec fs; fs.usePar( iop );
	ttl += " '"; ttl += getLimitedDisplayString(fs.fname_,40,0); ttl += "'";
	setTitleText( ttl );
    }

    uiSeparator* sep = optsgrp_ ? new uiSeparator( this, "Hor sep" ) : 0;

    uiGroup* outgrp = new uiGroup( this, "Output group" );
    transffld_ = new uiSeisTransfer( outgrp, uiSeisTransfer::Setup(setup_.geom_)
				    .withnullfill(false)
				    .fornewentry(true) );
    outgrp->setHAlignObj( transffld_ );
    if ( sep )
    {
	sep->attach( stretchedBelow, optsgrp_ );
	outgrp->attach( alignedBelow, optsgrp_ );
	outgrp->attach( ensureBelow, sep );
    }

    uiSeisSel::Setup sssu( setup_.geom_ ); sssu.enabotherdomain( true );
    seissel_ = new uiSeisSel( outgrp, ctio_, sssu );
    seissel_->attach( alignedBelow, transffld_ );

    if ( setup_.geom_ == Seis::Line )
    {
	morebut_ = new uiCheckBox( outgrp, "Import more, similar files" );
	morebut_->attach( alignedBelow, seissel_ );
    }

    if ( !optsgrp_ )
    {
	uiToolButton* tb = new uiToolButton( this, "prescan.png",
				"Pre-scan file(s)",
				mCB(this,uiSEGYImpDlg,preScanCB) );
	tb->attach( rightOf, outgrp->attachObj() );
    }
}


uiSEGYImpDlg::~uiSEGYImpDlg()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiSEGYImpDlg::use( const IOObj* ioobj, bool force )
{
    uiSEGYReadDlg::use( ioobj, force );
    if ( ioobj )
	transffld_->updateFrom( *ioobj );
}


class uiSEGYImpSimilarDlg : public uiDialog
{
public:

uiSEGYImpSimilarDlg( uiSEGYImpDlg* p, const IOObj& iio, const IOObj& oio,
		     const char* anm )
	: uiDialog(p,uiDialog::Setup("2D SEG-Y multi-import",
		    		     "Specify file details","103.0.4"))
	, inioobj_(iio)
	, outioobj_(oio)
	, impdlg_(p)
	, attrnm_(anm)
{
    const BufferString fnm( inioobj_.fullUserExpr(true) );
    FilePath fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Imp "); setupnm += uiSEGYFileSpec::sKeyLineNmToken();

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken() );
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    BufferString txt( "Input ('" ); txt += uiSEGYFileSpec::sKeyLineNmToken();
    txt += "' will become line name)";
    fnmfld_ = new uiFileInput( this, txt,
		    uiFileInput::Setup(fp.fullPath()).forread(true) );
}


bool acceptOK( CallBacker* )
{
    BufferString fnm = fnmfld_->fileName();
    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    if ( !File::isDirectory(dirnm) )
    {
	uiMSG().error( "Directory provided not usable" );
	return false;
    }
    if ( !strstr(fp.fullPath().buf(),uiSEGYFileSpec::sKeyLineNmToken()) )
    {
	BufferString msg( "The file name has to contain at least one '" );
	msg += uiSEGYFileSpec::sKeyLineNmToken(); msg += "'\n";
	msg += "That will then become the line name";
	uiMSG().error( msg );
	return false;
    }

    IOM().to( outioobj_.key() );
    return doImp( fp );
}


IOObj* getSubstIOObj( const char* fullfnm )
{
    IOObj* newioobj = inioobj_.clone();
    newioobj->setName( fullfnm );
    mDynamicCastGet(IOStream*,iostrm,newioobj)
    iostrm->setFileName( fullfnm );
    return newioobj;
}


bool doWork( IOObj* newioobj, const char* lnm, bool islast, bool& nofails )
{
    bool res = impdlg_->impFile( *newioobj, outioobj_, lnm, attrnm_ );
    delete newioobj;
    if ( !res )
    {
	nofails = false;
	if ( !islast && !uiMSG().askContinue("Continue with next?") )
	    return false;
    }
    return true;
}


bool doImp( const FilePath& fp )
{
    BufferString mask( fp.fileName() );
    replaceString( mask.buf(), uiSEGYFileSpec::sKeyLineNmToken(), "*" );
    FilePath maskfp( fp ); maskfp.setFileName( mask );
    const int nrtok = countCharacter( mask.buf(), '*' );
    DirList dl( fp.pathOnly(), DirList::FilesOnly, mask );
    if ( dl.size() < 1 )
    {
	uiMSG().error( "Cannot find any match for file name" );
	return false;
    }

    BufferString fullmaskfnm( maskfp.fullPath() );
    int lnmoffs = strstr( fullmaskfnm.buf(), "*" ) - fullmaskfnm.buf();
    const int orglen = fullmaskfnm.size();
    bool nofails = true;

    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString dirlistfnm( dl.get(idx) );
	FilePath newfp( maskfp );
	newfp.setFileName( dirlistfnm );
	const BufferString fullfnm( newfp.fullPath() );
	const int newlen = fullfnm.size();
	const int lnmlen = (newlen - orglen + 1) / nrtok;
	BufferString lnm( fullfnm.buf() + lnmoffs );
	*(lnm.buf() + lnmlen) = '\0';

	IOObj* newioobj = getSubstIOObj( fullfnm );
	if ( !doWork( newioobj, lnm, idx > dl.size()-2, nofails ) )
	    return false;
    }

    return nofails;
}

    uiFileInput*	fnmfld_;
    uiSEGYImpDlg*	impdlg_;

    const IOObj&	inioobj_;
    const IOObj&	outioobj_;
    const char*		attrnm_;

};



bool uiSEGYImpDlg::doWork( const IOObj& inioobj )
{
    if ( !seissel_->commitInput() )
    {
	if ( seissel_->isEmpty() )
	    uiMSG().error( "Please select the output data" );
	return false;
    }

    const IOObj& outioobj = *ctio_.ioobj;
    const bool is2d = Seis::is2D( setup_.geom_ );
    const char* attrnm = seissel_->attrNm();
    const char* lnm = is2d && transffld_->selFld2D() ?
		      transffld_->selFld2D()->selectedLine() : 0;

    const IOObj* useinioobj = &inioobj; IOObj* tmpioobj = 0;
    const bool outissidom = ZDomain::isSI( outioobj.pars() );
    if ( !outissidom )
    {
	tmpioobj = inioobj.clone();
	ZDomain::Def::get(outioobj.pars()).set( tmpioobj->pars() );
	useinioobj = tmpioobj;
    }

    bool retval;
    if ( !morebut_ || !morebut_->isChecked() )
    {
	retval = impFile( *useinioobj, outioobj, lnm, attrnm );
	if ( is2d && retval )
	    uiMSG().message( "Successfully loaded ",
		    		useinioobj->fullUserExpr() );
    }
    else
    {
	uiSEGYImpSimilarDlg dlg( this, *useinioobj, outioobj, attrnm );
	retval = dlg.go();
    }

    if ( tmpioobj )
	IOM().commitChanges( *tmpioobj );
    delete tmpioobj;
    return retval;
}


bool uiSEGYImpDlg::impFile( const IOObj& inioobj, const IOObj& outioobj,
				const char* linenm, const char* attrnm )
{
    const bool isps = Seis::isPS( setup_.geom_ );
    const bool is2d = Seis::is2D( setup_.geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo;
    if ( !isps )
    {
	ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
	if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	    return false;
    }

    if ( is2d )
    {
	SeisIOObjInfo seisinfo( outioobj );
	SeisIOObjInfo::Opts2D option;
	option.steerpol_ = 0;
	BufferStringSet attrnms;
	seisinfo.getAttribNamesForLine( linenm, attrnms );
	if ( attrnms.size()==1 )
	{
	    BufferString msg(
		    "Geometry of Line '", linenm,
		    "' is already present. Do you want to overwrite?" ); 
	    if ( uiMSG().askGoOn(msg) )
	    {
		S2DPOS().setCurLineSet( outioobj.name() );
		PosInfo::POS2DAdmin().removeLine( linenm );
	    }
	}
    }

    SEGY::TxtHeader::info2D() = is2d;
    transffld_->scfmtfld->updateIOObj( const_cast<IOObj*>(&outioobj), true );
    PtrMan<SeisTrcWriter> wrr = new SeisTrcWriter( &outioobj );
    SeisStdImporterReader* rdr = new SeisStdImporterReader( inioobj, "SEG-Y" );
    rdr->removeNull( transffld_->removeNull() );
    rdr->setResampler( transffld_->getResampler() );
    rdr->setScaler( transffld_->scfmtfld->getScaler() );
    Seis::SelData* sd = transffld_->getSelData();
    if ( is2d )
    {
	if ( linenm && *linenm )
	    sd->lineKey().setLineName( linenm );
	if ( !isps )
	    sd->lineKey().setAttrName( attrnm );
	wrr->setSelData( sd->clone() );
    }
    rdr->setSelData( sd );

    PtrMan<SeisImporter> imp = new SeisImporter( rdr, *wrr, setup_.geom_ );
    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( imp->name() );
	nm += " ("; nm += linenm; nm += ")";
	imp->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = dlg.execute( *imp );
    BufferStringSet warns;
    if ( imp && imp->nrSkipped() > 0 )
	warns += new BufferString("During import, ", imp->nrSkipped(),
				  " traces were rejected" );
    SeisTrcTranslator* tr = rdr->reader().seisTranslator();
    if ( tr && tr->haveWarnings() )
	warns.add( tr->warnings(), false );
    imp.erase(); wrr.erase(); // closes output cube

    displayWarnings( warns );
    if ( rv && !is2d && ioobjinfo )
	rv = ioobjinfo->provideUserInfo();

    SEGY::TxtHeader::info2D() = false;
    return rv;
}
