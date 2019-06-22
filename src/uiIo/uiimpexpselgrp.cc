/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Nov 2010
________________________________________________________________________

-*/

#include "uiimpexpselgrp.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeom.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitoolbutton.h"

#include "ascstream.h"
#include "bufstring.h"
#include "color.h"
#include "ioobjctxt.h"
#include "file.h"
#include "keystrs.h"
#include "odver.h"
#include "safefileio.h"
#include "separstr.h"
#include "od_iostream.h"
#include "dbman.h"
#include "od_helpids.h"

static const char* sKeyFileType = "CrossPlot Selection";
static const char* sKeyNrSelGrps = "Nr of Selection Groups";
static const char* sKeySelGrp()		{ return "SelectionGrps"; }
static const char* sKeyIdxFileName()	{ return "index.txt"; }


class SGSelGrpManager
{ mODTextTranslationClass(SGSelGrpManager);

public:

SGSelGrpManager()
{ createBaseDir(); }


~SGSelGrpManager()
{ delete &basefp_; }


bool renameSelGrpSet( const char* oldnm, const char* newnm )
{
    BufferStringSet nms;
    getSelGrpSetNames( nms );
    const int sgidx = nms.indexOf( oldnm );
    if ( mIsUdf(sgidx) || sgidx<0 )
	return false;

    BufferString oldclnnm( oldnm );
    oldclnnm.clean();
    BufferString newclnnm( oldnm );
    newclnnm.clean();

    File::Path newfp( basefp_.fullPath(), newclnnm );
    File::Path oldfp( basefp_.fullPath(), oldclnnm );
    nms.get( sgidx ) = newnm;
    setSelGrpSetNames( nms );
    File::rename( oldfp.fullPath(), newfp.fullPath() );
    return true;
}


bool addSelGrpSet( const char* nm )
{
    if ( !nm || !*nm )
	return false;

    BufferStringSet nms;
    if ( !getSelGrpSetNames(nms) || nms.isPresent(nm) )
	return false;
    nms.add( nm );
    if ( !setSelGrpSetNames(nms) )
	return false;
    return true;
}


bool deleteSelGrpSet( const char* nm )
{
    BufferStringSet nms;
    getSelGrpSetNames( nms );
    const int sgidx = nms.indexOf( nm );
    if ( sgidx<0 || mIsUdf(sgidx) )
	return false;

    nms.removeSingle( sgidx );
    setSelGrpSetNames( nms );
    return File::remove( File::Path(basefp_,nm).fullPath() );
}


BufferString basePath() const
{ return basefp_.fullPath(); }


bool hasIdxFile()
{
    return File::exists( File::Path(basefp_,sKeyIdxFileName()).fullPath() );
}


bool createBaseDir()
{
    basefp_ = IOObjContext::getDataDirName( IOObjContext::Feat );
    basefp_.add( sKeySelGrp() );

    if ( !File::exists(basefp_.fullPath()) )
    {
	if  ( !File::createDir(basefp_.fullPath()) )
	{
	    const uiString msg =
		uiStrings::phrCannotCreateDirectory( basefp_.fullPath() );
	    gUiMsg().error( msg );
	    return false;
	}
    }

    return true;
}


bool setSelGrpSetNames( const BufferStringSet& nms )
{
    const uiString filtypstr = tr("Cross-Plot Selection Index File");

    SafeFileIO sfio( File::Path(basefp_,sKeyIdxFileName()).fullPath(), true );
    if ( !sfio.open(false) )
	{ gUiMsg().error( uiStrings::phrCannotWrite(filtypstr) ); return false;}

    ascostream astrm( sfio.ostrm() );
    astrm.putHeader( "Selection Group Set Names" );
    for ( int idx=0; idx<nms.size(); idx++ )
	astrm.put( nms.get(idx).buf() );
    astrm.newParagraph();

    if ( sfio.ostrm().isOK() )
	sfio.closeSuccess();
    else
    {
	sfio.closeFail();
	uiString errmsg( uiStrings::phrErrDuringWrite(filtypstr) );
	sfio.ostrm().addErrMsgTo( errmsg );
	gUiMsg().error( errmsg );
	return false;
    }

    return true;
}


bool getSelGrpSetNames( BufferStringSet& nms )
{
    File::Path fp( basefp_,sKeyIdxFileName() );
    if ( !File::exists(fp.fullPath()) )
    {
	BufferStringSet emptynms;
	if ( !setSelGrpSetNames(emptynms) )
	    return false;
    }

    SafeFileIO sfio( File::Path(basefp_,sKeyIdxFileName()).fullPath(), true );
    if ( !sfio.open(true) )
	return false;
    ascistream astrm( sfio.istrm() );
    if ( atEndOfSection(astrm) )
	astrm.next();

    while ( !atEndOfSection(astrm) )
    {
	nms.add( astrm.keyWord() );
	astrm.next();
    }

    sfio.closeSuccess();
    return true;
}

    File::Path	    basefp_;
};


static SGSelGrpManager* sgm = 0;

struct SGSelGrpManDeleter : public CallBacker
{
void doDel( CallBacker* )
{ delete sgm; sgm = 0; }
};


static SGSelGrpManager& SGM()
{
    if ( !sgm )
    {
	sgm = new SGSelGrpManager();
	mDefineStaticLocalObject( SGSelGrpManDeleter, sgsmd, );
	DBM().surveyToBeChanged.notify( mCB(&sgsmd,SGSelGrpManDeleter,doDel) );
    }
    return *sgm;
}


uiSGSelGrp::uiSGSelGrp( uiParent* p, bool forread )
    : uiGroup(p)
    , forread_(forread)
    , selectionDone(this)
{
    listfld_ = new uiListBox( this );
    listfld_->selectionChanged.notify( mCB(this,uiSGSelGrp,selChangedCB) );
    listfld_->doubleClicked.notify( mCB(this,uiSGSelGrp,selDoneCB) );

    if ( !forread )
    {
	nmfld_ = new uiGenInput( this, uiStrings::sName() );
	nmfld_->attach( alignedBelow, listfld_ );
	nmfld_->setElemSzPol( uiObject::SmallMax );
	nmfld_->setStretch( 2, 0 );
    }
    uiString trselgrp = tr("Selection-Groups");
    infobut_ = new uiToolButton( this, "info", uiStrings::sInfo(),
				 mCB(this,uiSGSelGrp,showInfo) );
    infobut_->attach( rightTo, listfld_ );

    delbut_ = new uiToolButton( this, "delete", uiStrings::phrDelete(trselgrp),
			        mCB(this,uiSGSelGrp,delSelGrps) );
    delbut_->attach( alignedBelow, infobut_ );

    renamebut_ = new uiToolButton( this, "renameobj",uiStrings::phrRename(
			        trselgrp), mCB(this,uiSGSelGrp,renameSelGrps) );
    renamebut_->attach( alignedBelow, delbut_ );

    fillListBox();
}


void uiSGSelGrp::selDoneCB( CallBacker* )
{
    selectionDone.trigger();
}


void uiSGSelGrp::selChangedCB( CallBacker* )
{
    if ( !forread_ )
	nmfld_->setText( listfld_->getText() );
}


void uiSGSelGrp::showInfo( CallBacker* )
{
    uiString info;
    ObjectSet<SelectionGrp> selgrpset;
    getCurSelGrpSet( selgrpset );

    for ( int idx=0; idx<selgrpset.size(); idx++ )
	selgrpset[idx]->getInfo( info );

    deepErase( selgrpset );
    uiMSG().message( info );
}


void uiSGSelGrp::delSelGrps( CallBacker* )
{
    BufferStringSet nms;
    SGM().getSelGrpSetNames( nms );
    const int idx = nms.indexOf( listfld_->getText() );
    if ( mIsUdf(idx) || idx < 0 )
	return;

    SGM().deleteSelGrpSet( listfld_->getText() );
    fillListBox();
}



class uiRenameDlg : public uiDialog
{ mODTextTranslationClass(uiRenameDlg);
public:
uiRenameDlg( uiParent* p, const char* nm )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrRename(
				 tr("Selection Group Set")),
                                 uiString::empty(), mNoHelpKey) )
{
    namefld_ = new uiGenInput( this, tr("Selection Group Set Name") );
    namefld_->setText( nm );
}

const char* getNewName()
{ return namefld_->text(); }

    uiGenInput*			namefld_;

};


void uiSGSelGrp::renameSelGrps( CallBacker* )
{
    BufferStringSet nms;
    SGM().getSelGrpSetNames( nms );
    uiRenameDlg dlg( this, listfld_->getText() );
    if ( dlg.go() )
    {
	const int idx = nms.indexOf( listfld_->getText() );
	if ( mIsUdf(idx) || idx < 0 || !dlg.getNewName() )
	    return;
	SGM().renameSelGrpSet( listfld_->getText(), dlg.getNewName() );
	fillListBox();
    }
}


bool uiSGSelGrp::fillListBox()
{
    BufferStringSet selgrpsetnms;
    if ( !SGM().getSelGrpSetNames(selgrpsetnms) )
	return false;

    listfld_->setEmpty();
    listfld_->addItems( selgrpsetnms );

    return true;
}


bool uiSGSelGrp::getCurSelGrpSet( ObjectSet<SelectionGrp>& selgrps )
{
    SelGrpImporter imp( getCurFileNm() );
    selgrps = imp.getSelections();

    xname_ = imp.xName();
    yname_ = imp.yName();
    y2name_ = imp.y2Name();

    if ( !imp.errMsg().isEmpty() )
    {
	uiMSG().error(imp.errMsg());
	return false;
    }

    return true;
}


const char* uiSGSelGrp::selGrpSetNm() const
{
    return forread_ ? listfld_->getText() : nmfld_->text();
}


BufferString uiSGSelGrp::getCurFileNm() const
{
    BufferString cleannm( forread_ ? listfld_->getText() : nmfld_->text() );
    cleannm.clean();
    return File::Path(SGM().basePath(),cleannm).fullPath();
}


SelGrpImporter::SelGrpImporter( const char* fnm )
{
    strm_ = new od_istream( fnm );
    if ( !strm_->isOK() )
    {
	errmsg_ = tr("Cannot open specified file");
	strm_->addErrMsgTo( errmsg_ );
	delete strm_; strm_ = 0;
    }
}


SelGrpImporter::~SelGrpImporter()
{
    delete strm_;
}


ObjectSet<SelectionGrp> SelGrpImporter::getSelections()
{
    ObjectSet<SelectionGrp> selgrpset;
    if ( !strm_ )
	return selgrpset;

    ascistream astrm( *strm_, true );

    if ( !astrm.isOfFileType(sKeyFileType) )
    {
	errmsg_ = tr("File type does not match with Cross-plot Selection");
	return selgrpset;
    }

    int nrselgrps = 0;
    IOPar par( astrm );

    if ( par.hasKey(sKeyNrSelGrps) )
	par.get( sKeyNrSelGrps, nrselgrps );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute(),sKey::X())) )
	par.get( IOPar::compKey(sKey::Attribute(),sKey::X()), xname_ );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute(),sKey::Y())) )
	par.get( IOPar::compKey(sKey::Attribute(),sKey::Y()), yname_ );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute(),sKey::Y2())) )
	par.get( IOPar::compKey(sKey::Attribute(),sKey::Y2()), y2name_ );

    for ( int selidx=0; selidx < nrselgrps; selidx++ )
    {
	PtrMan<IOPar> selgrppar = par.subselect( selidx );
	if ( !selgrppar ) continue;

	SelectionGrp* selgrp = new SelectionGrp();
	selgrp->usePar( *selgrppar );
	selgrpset += selgrp;
    }

    delete strm_; strm_ = 0;
    return selgrpset;
}


SelGrpExporter::SelGrpExporter( const char* fnm )
{
    strm_ = new od_ostream( fnm );
    if ( !strm_->isOK() )
    {
	errmsg_ = tr("Cannot write to specified file");
	strm_->addErrMsgTo( errmsg_ );
	delete strm_; strm_ = 0;
    }
}

SelGrpExporter::~SelGrpExporter()
{
    delete strm_;
}

bool SelGrpExporter::putSelections( const ObjectSet<SelectionGrp>& selgrps,
				    const char* xname, const char* yname,
				    const char* y2name )
{
    if ( !strm_ )
	return false;

    ascostream astrm( *strm_ );

    if ( !selgrps.size() )
	{ errmsg_ = tr("No selections found"); return false; }

    IOPar selectionpar;
    selectionpar.set( IOPar::compKey(sKey::Attribute(),sKey::X()), xname );
    selectionpar.set( IOPar::compKey(sKey::Attribute(),sKey::Y()), yname );
    if ( y2name )
	selectionpar.set( IOPar::compKey(sKey::Attribute(),sKey::Y2()), y2name);
    selectionpar.set( sKeyNrSelGrps, selgrps.size() );

    for ( int selidx=0; selidx<selgrps.size(); selidx++ )
    {
	const SelectionGrp* selgrp = selgrps[selidx];
	IOPar par,selgrppar;
	BufferString selstr;
	selstr.add( selidx );
	selgrp->fillPar( par );
	selgrppar.mergeComp( par, selstr.buf() );
	selectionpar.merge( selgrppar );
    }

    selectionpar.write( astrm.stream(), sKeyFileType );
    const bool ret = astrm.isOK();
    if ( !ret )
    {
	errmsg_ = tr("Error during write");
	strm_->addErrMsgTo( errmsg_ );
    }

    delete strm_; strm_ = 0;
    return ret;
}


class uiSGSelDlg : public uiDialog
{ mODTextTranslationClass(uiSGSelDlg)
public:

uiSGSelDlg( uiParent* p, bool forread )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSelect(
				    tr("CrossPlot Selection Groups")),
				    uiString::empty(), mNoHelpKey))
    , forread_(forread)
{
    selgrp_ = new uiSGSelGrp( this, forread );
    selgrp_->selectionDone.notify( mCB(this,uiSGSelDlg,accept) );
}


bool acceptOK()
{
    bool ret = true;
    if ( forread_ )
	ret = selgrp_->getCurSelGrpSet( selgrpset_ );
    else
	SGM().addSelGrpSet( selgrp_->selGrpSetNm() );

    selgrpsetnm_ = selgrp_->selGrpSetNm();
    filenm_ =  selgrp_->getCurFileNm();
    xname_ = selgrp_->xName();
    yname_ = selgrp_->yName();
    y2name_ = selgrp_->y2Name();

    return ret;
}

const ObjectSet<SelectionGrp>& selGrpSet() const	{ return selgrpset_; }

const char* selGrSetNm() const				{ return selgrpsetnm_; }
const OD::String& selGrpFileNm() const			{ return filenm_;}

const char* xName() const				{ return xname_; }
const char* yName() const				{ return yname_; }
const char* y2Name() const				{ return y2name_; }

protected:

    ObjectSet<SelectionGrp>	selgrpset_;
    uiSGSelGrp*		selgrp_;

    BufferString		filenm_;
    BufferString		xname_;
    BufferString		yname_;
    BufferString		y2name_;
    bool			forread_;

    BufferString		selgrpsetnm_;
};



uiSGSel::uiSGSel( uiParent* p, bool forread )
    : uiGroup(p)
    , forread_(forread)
    , selGrpSelected(this)
{
    inpfld_ = new uiGenInput( this, tr("Cross-plot Selections") );
    selbut_ = new uiPushButton( this,
		    m3Dots(uiStrings::sSelect() ),
		    mCB(this,uiSGSel,selectSGCB), false );
    selbut_->attach( rightTo, inpfld_ );
}


void uiSGSel::selectSGCB( CallBacker* )
{
    uiSGSelDlg dlg( this, forread_ );
    if ( dlg.go() )
    {
	inpfld_->setText( dlg.selGrSetNm() );
	selgrpfilenm_ = dlg.selGrpFileNm();
	selgrpset_ = dlg.selGrpSet();
	xname_ = dlg.xName();
	yname_ = dlg.yName();
	y2name_ = dlg.y2Name();
	selGrpSelected.trigger();
    }
}


bool uiSGSel::isOK() const
{
    return inpfld_->text() || !selgrpfilenm_.isEmpty();
}


const char* uiSGSel::selGrpSetNm() const
{
    return inpfld_->text();
}


const char* uiSGSel::selGrpFileNm()
{
    if ( !isOK() )
	return 0;


    if ( selgrpfilenm_.isEmpty() )
    {
	BufferString cleannm( inpfld_->text() );
	cleannm.clean();
	File::Path fp( SGM().basePath(), cleannm );
	mDefineStaticLocalObject( BufferString, selgrpfnm, (fp.fullPath()) );
	return selgrpfnm;
	//selgrpfilenm_ = File::Path(SGM().basePath(),cleannm).fullPath();
    }

    return selgrpfilenm_.buf();
}


uiReadSelGrp::uiReadSelGrp( uiParent* p, uiDataPointSetCrossPlotter& plotter )
    : uiDialog(p,uiDialog::Setup(tr("Open Cross Plot Selection"), mNoDlgTitle,
				 mODHelpKey(mReadSelGrpHelpID) ))
    , plotter_(plotter)
    , selgrpset_(plotter.selectionGrps())
    , y2selfld_(0)
{
    bool hasy2 = plotter.axisHandler(2);
    BufferStringSet nms;
    nms.add( toString(plotter.axisHandler(0)->getCaption()) );
    nms.add( toString(plotter.axisHandler(1)->getCaption()) );
    if ( hasy2 )
	nms.add( toString(plotter.axisHandler(2)->getCaption()) );

    inpfld_ = new uiSGSel( this, true );
    inpfld_->selGrpSelected.notify( mCB(this,uiReadSelGrp,selectedCB) );

    uiLabeledComboBox* xselfld =
	new uiLabeledComboBox( this,
		    plotter.axisHandler(0)->getCaption() );
    xselfld_ = xselfld->box();
    xselfld->attach( centeredBelow, inpfld_ );
    xselfld_->display( false, false );

    uiLabeledComboBox* yselfld =
	new uiLabeledComboBox( this,
		    plotter.axisHandler(1)->getCaption() );
    yselfld_ = yselfld->box();
    yselfld->attach( alignedBelow, xselfld );
    yselfld_->display( false, false );

    ychkfld_ = new uiCheckBox( this, uiStrings::phrImport(uiStrings::sY1()),
			       mCB(this,uiReadSelGrp,fldCheckedCB) );
    ychkfld_->attach( rightTo, yselfld );
    ychkfld_->display( false, false );

    if ( hasy2 )
    {
	uiLabeledComboBox* y2selfld =
	    new uiLabeledComboBox( this,
			plotter.axisHandler(2)->getCaption() );
	y2selfld_ = y2selfld->box();
	y2selfld->attach( alignedBelow, yselfld );
	y2selfld_->display( false, false );
	y2chkfld_ = new uiCheckBox( this, uiStrings::sY2(),
				    mCB(this,uiReadSelGrp,fldCheckedCB) );
	y2chkfld_->attach( rightTo, y2selfld );
	y2chkfld_->display( false, false );
    }
}


void uiReadSelGrp::fldCheckedCB( CallBacker* cb )
{
    mDynamicCastGet(uiCheckBox*,chkbox,cb);
    if ( ychkfld_ == chkbox )
	yselfld_->setSensitive( ychkfld_->isChecked() );
    else if ( y2chkfld_ == chkbox )
	y2selfld_->setSensitive( y2chkfld_->isChecked() );
}


#define mSetMatchingItem( selfld, idx ) \
    matchidx = nms.nearestMatch( axisnms[idx]->buf() ); \
    if ( matchidx >= 0 ) \
	selfld->setCurrentItem( matchidx ); \

void uiReadSelGrp::selectedCB( CallBacker* )
{
    if ( !inpfld_->isOK() )
    {
	uiMSG().error(tr("Selected Selection-Group set is corrupted"));
	return;
    }

    selgrpset_ = inpfld_->selGrpSet();

    BufferStringSet axisnms;
    axisnms.add( toString(plotter_.axisHandler(0)->getCaption()) );
    axisnms.add( toString(plotter_.axisHandler(1)->getCaption()) );
    if ( plotter_.axisHandler(2) )
	axisnms.add( toString(plotter_.axisHandler(2)->getCaption()) );

    xname_ = inpfld_->xName();
    yname_ = inpfld_->yName();
    y2name_ = inpfld_->y2Name();

    int matchidx = 0;
    BufferStringSet nms;
    nms.add( xname_ );
    nms.add( yname_ );
    if ( !y2name_.isEmpty() )
	nms.add( y2name_ );
    xselfld_->setEmpty();
    xselfld_->addItems( nms );
    mSetMatchingItem( xselfld_, 0 );
    xselfld_->display( true );

    yselfld_->setEmpty();
    yselfld_->addItems( nms );
    mSetMatchingItem( yselfld_, 1 );
    yselfld_->display( true );
    ychkfld_->display( false );
    ychkfld_->setChecked( true );
    yselfld_->setSensitive( true );

    if ( y2selfld_ )
    {
	ychkfld_->display( true );
	y2selfld_->setEmpty();
	y2selfld_->addItems( nms );
	mSetMatchingItem( y2selfld_, 2 );
	y2selfld_->display( true );
	y2selfld_->setSensitive( false );
	y2chkfld_->display( true );
    }
}


BufferStringSet uiReadSelGrp::getAvailableAxisNames() const
{
    BufferStringSet axisnm;
    axisnm.add( xselfld_->itemText(xselfld_->currentItem()) );
    if ( ychkfld_->isChecked() || !ychkfld_->isDisplayed() )
	axisnm.add( yselfld_->itemText(yselfld_->currentItem()) );
    if ( y2selfld_ && y2chkfld_->isChecked() )
	axisnm.add( y2selfld_->itemText(y2selfld_->currentItem()) );
    return axisnm;
}

#define mGetAxisVals \
    const int xaxis mUnusedVar = xselfld_->currentItem(); \
    int yaxis=-1; \
    int y2axis=-2; \
    if ( !ychkfld_->isDisplayed() || ychkfld_->isChecked() ) \
	yaxis = yselfld_->currentItem(); \
    if ( y2selfld_ ) \
    { \
	if ( y2chkfld_->isChecked() ) \
	    y2axis = y2selfld_->currentItem(); \
    }



bool uiReadSelGrp::checkSelectionArea( SelectionArea& actselarea,
				       const BufferStringSet& impaxisnm,
				       const BufferStringSet& xpaxisnms,
				       bool hasalt )
{
    mGetAxisVals;

    if ( !impaxisnm.isPresent(xpaxisnms.get(0)) )
	return false;

    if ( yaxis<0 && y2axis>=0 )
    {
	if ( (xpaxisnms.validIdx(2) && !impaxisnm.isPresent(xpaxisnms.get(2)))
	     || !impaxisnm.isPresent(xpaxisnms.get(1))  )
	    return false;

	actselarea.axistype_ = SelectionArea::Y2;
    }
    else if ( yaxis>=0 && y2axis<0 )
    {
	if ( !impaxisnm.isPresent(xpaxisnms.get(1)) )
	    return false;

	actselarea.axistype_ = SelectionArea::Y1;
    }
    else if ( yaxis>=0 && y2axis>=0 )
    {
	actselarea.axistype_ = SelectionArea::Both;
	if ( !hasalt )
	{
	    if ( !impaxisnm.isPresent(xpaxisnms.get(1)) )
		actselarea.axistype_ = SelectionArea::Y2;
	    else if ( !impaxisnm.isPresent(xpaxisnms.get(2)) )
		actselarea.axistype_ = SelectionArea::Y1;
	}
    }

    return true;
}


void uiReadSelGrp::fillRectangle( const SelectionArea& selarea,
				  SelectionArea& actselarea )
{
    mGetAxisVals;
    bool hasalt = !selarea.altyaxisnm_.isEmpty();
    if ( xaxis == 0 )
    {
	actselarea.worldrect_ =
	    ((yaxis == 2) && hasalt) ? selarea.altworldrect_
				     : selarea.worldrect_;
	actselarea.altworldrect_ =
	    ((yaxis == 2) && hasalt) ? selarea.worldrect_
				     : selarea.altworldrect_;
    }
    else
    {
	uiWorldRect rect = selarea.worldrect_;
	uiWorldRect altrect = hasalt ? selarea.altworldrect_
				    : selarea.worldrect_;
	TypeSet<double> ltptval;
	ltptval += rect.topLeft().x_;
	ltptval += rect.topLeft().y_;
	ltptval += altrect.topLeft().y_;

	TypeSet<double> rbptval;
	rbptval += rect.bottomRight().x_;
	rbptval += rect.bottomRight().y_;
	rbptval += altrect.bottomRight().y_;

	const bool onlyy2 = actselarea.axistype_ == SelectionArea::Y2;
	const int yaxisnr = (yaxis<0 || onlyy2) ? y2axis : yaxis;

	actselarea.worldrect_ =
	   uiWorldRect( ltptval[xaxis], ltptval[yaxisnr],
			rbptval[xaxis], rbptval[yaxisnr] );
	actselarea.worldrect_.sortCorners( true, false );
	if (hasalt && actselarea.axistype_==SelectionArea::Both)
	{
	   actselarea.altworldrect_ =
	       uiWorldRect( ltptval[xaxis], ltptval[y2axis],
			    rbptval[xaxis], rbptval[y2axis] );
	   actselarea.altworldrect_.sortCorners( true, false );
	}
    }
}


void uiReadSelGrp::fillPolygon( const SelectionArea& selarea,
			        SelectionArea& actselarea )
{
    mGetAxisVals;
    bool hasalt = !selarea.altyaxisnm_.isEmpty();

    if ( xaxis == 0 )
    {
	actselarea.worldpoly_ = ((yaxis) == 2 && hasalt)
	    ? selarea.altworldpoly_ : selarea.worldpoly_;
	actselarea.altworldpoly_ = ((yaxis == 2) && hasalt)
	    ? selarea.worldpoly_ : selarea.altworldpoly_;
    }
    else
    {
	ODPolygon<double> worldpoly,altworldpoly;
	TypeSet< Geom::Point2D<double> > pts = selarea.worldpoly_.data();
	TypeSet< Geom::Point2D<double> > altpts =
				   hasalt ? selarea.altworldpoly_.data()
					  : selarea.worldpoly_.data();
	for ( int idx=0; idx<pts.size(); idx++ )
	{
	   TypeSet<double> ptval;
	   ptval += pts[idx].x_; ptval += pts[idx].y_;
	   ptval += altpts[idx].y_;
	   const bool onlyy2 = actselarea.axistype_ == SelectionArea::Y2;
	   const int yaxisnr = (yaxis<0 || onlyy2) ? y2axis : yaxis;

	   worldpoly.add( Geom::Point2D<double>(ptval[xaxis], ptval[yaxisnr]) );

	   if (hasalt && actselarea.axistype_==SelectionArea::Both)
	       altworldpoly.add( Geom::Point2D<double>(ptval[xaxis],
				 ptval[y2axis]) );
	}

	actselarea.worldpoly_ = worldpoly;
	actselarea.altworldpoly_ = altworldpoly;
    }
}


bool uiReadSelGrp::adjustSelectionGrps()
{
    mGetAxisVals;
    if ( xaxis<0 || (yaxis<0 && y2axis<0) )
    {
	uiMSG().error(tr("Can't import selection group"));
	return false;
    }

    if ( xaxis==yaxis || yaxis==y2axis || y2axis==xaxis )
    {
	uiMSG().error(tr("Same parameter chosen for different axis"));
	return false;
    }

    if ( yaxis < 0 && !plotter_.axisHandler(2) )
    {
	uiMSG().error(tr("Choose axis properly"));
	return false;
    }

    BufferStringSet xpaxisnms = getAvailableAxisNames();

    int selareaid = 0;
    bool selimportfailed = false;
    for ( int selidx=0; selidx<selgrpset_.size(); selidx++ )
    {
	SelectionGrp* selgrp = selgrpset_[selidx];

	SelectionGrp* newselgrp =
	    new SelectionGrp( selgrp->name(), selgrp->col_ );
	for ( int idx=0; idx<selgrp->size(); idx++ )
	{
	    const SelectionArea& selarea = selgrp->getSelectionArea( idx );
	    SelectionArea actselarea = SelectionArea( selarea.isrectangle_ );
	    bool hasalt = !selarea.altyaxisnm_.isEmpty();

	    if ( !checkSelectionArea(actselarea,selarea.getAxisNames(),
				     xpaxisnms,hasalt) )
	    {
		selimportfailed = true;
		continue;
	    }

	    if ( selarea.isrectangle_ )
		fillRectangle( selarea, actselarea );
	    else
		fillPolygon( selarea, actselarea );

	    if ( plotter_.checkSelArea(actselarea) )
	    {
		actselarea.id_ = selareaid;
		actselarea.xaxisnm_ =
			toString( plotter_.axisHandler(0)->getCaption() );
		actselarea.yaxisnm_ = toString(
		    plotter_.axisHandler( yaxis < 0 ? 2 : 1 )->getCaption() );
		if ( hasalt && y2axis >=0 )
		    actselarea.altyaxisnm_ =
		    toString( plotter_.axisHandler(2)->getCaption() );
		newselgrp->addSelection( actselarea );
		selareaid++;
	    }
	    else
		selimportfailed = true;
	}

	delete selgrpset_.replace( selgrpset_.indexOf(selgrp), newselgrp );
    }

    if ( selimportfailed )
	uiMSG().warning(tr("Some selection areas could not be imported,\nas "
			   "they are outside the value ranges of the plot"));

    return true;
}


void uiReadSelGrp::getInfo( const ObjectSet<SelectionGrp>& selgrps,
			    BufferString& msg )
{
    for ( int selgrpidx=0; selgrpidx<selgrps.size(); selgrpidx++ )
    {
	selgrps[selgrpidx]->getInfo( msg );
	msg += "\n";
    }
}


bool uiReadSelGrp::acceptOK()
{
    if ( !adjustSelectionGrps() )
	return false;
    plotter_.reDrawSelections();
    return true;
}


uiExpSelectionArea::uiExpSelectionArea( uiParent* p,
					const ObjectSet<SelectionGrp>& selgrps,
					uiExpSelectionArea::Setup su )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSave(tr("Selection Area")),
				 uiStrings::phrSpecify(
				 uiStrings::sParameter(mPlural)),
                                 mODHelpKey(mExpSelectionAreaHelpID) ))
    , selgrps_(selgrps)
    , setup_(su)
{
    outfld_ = new uiSGSel( this, false );
}


bool uiExpSelectionArea::acceptOK()
{
    if ( !outfld_->isOK() )
    {
	uiMSG().error(uiStrings::phrSelect(tr("an ouput name")));
	return false;
    }

    if ( File::exists(outfld_->selGrpFileNm()) )
    {
	if (!uiMSG().askOverwrite(tr("Selected selections already present.\n\n"
				     "Do you want to overwrite?")))
	    return false;
    }

    SGM().addSelGrpSet( outfld_->selGrpSetNm() );
    FixedString sgfnm( outfld_->selGrpFileNm() );
    SelGrpExporter exp( sgfnm );
    BufferString yaxisnm;
    if ( !exp.putSelections(selgrps_,setup_.xname_,
			    setup_.yname_,setup_.y2name_) )
	{ uiMSG().error(exp.errMsg()); return false; }

    return true;
}
