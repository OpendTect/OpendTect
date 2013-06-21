/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Nov 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "";

#include "uiimpexpselgrp.h"

#include "uiaxishandler.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeom.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitoolbutton.h"

#include "ascstream.h"
#include "bufstring.h"
#include "color.h"
#include "ctxtioobj.h"
#include "file.h"
#include "odver.h"
#include "safefileio.h"
#include "separstr.h"
#include "strmprov.h"
#include "strmoper.h"
#include "survinfo.h"
#include "timefun.h"

static const char* sKeyFileType = "CrossPlot Selection";
static const char* sKeyNrSelGrps = "Nr of Selection Groups";
static const char* sKeySelGrp()		{ return "SelectionGrps"; }
static const char* sKeyIdxFileName() 	{ return "index.txt"; }


class SGSelGrpManager
{

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
    cleanupString( oldclnnm.buf(), false, false, false );
    BufferString newclnnm( oldnm );
    cleanupString( newclnnm.buf(), false, false, false );

    FilePath newfp( basefp_.fullPath(), newclnnm );
    FilePath oldfp( basefp_.fullPath(), oldclnnm );
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
    return File::remove( FilePath(basefp_,nm).fullPath() );
}


BufferString basePath() const
{ return basefp_.fullPath(); }


bool hasIdxFile()
{
    return File::exists( FilePath(basefp_,sKeyIdxFileName()).fullPath() );
}


bool createBaseDir()
{
    basefp_ = IOObjContext::getDataDirName( IOObjContext::Feat );
    basefp_.add( sKeySelGrp() );

    if ( !File::exists(basefp_.fullPath()) )
    {
	if  ( !File::createDir(basefp_.fullPath()) )
	{
	    BufferString msg( "Cannot create " );
	    msg += sKeySelGrp();
	    msg += "for crossplot selctions. Check write permissions";
	    uiMSG().error( msg );
	    return false;
	}
    }

    return true;
}


bool setSelGrpSetNames( const BufferStringSet& nms )
{
    SafeFileIO sfio( FilePath(basefp_,sKeyIdxFileName()).fullPath(), true );
    if ( !sfio.open(false) )
    {
	uiMSG().error("Cannot open Cross-plot Selection index.txt "
		      "file for write");
	return false;
    }

    ascostream astrm( sfio.ostrm() );
    astrm.putHeader( "Selection Group Set Names" );
    for ( int idx=0; idx<nms.size(); idx++ )
	astrm.put( nms.get(idx).buf() );
    astrm.newParagraph();

    if ( sfio.ostrm().good() )
	sfio.closeSuccess();
    else
    {
	sfio.closeFail();
	uiMSG().error( "Error during write to Cross-plot Selection index file ."
		       "Check disk space." );
	return false;
    }

    return true;
}


bool getSelGrpSetNames( BufferStringSet& nms )
{
    FilePath fp( basefp_,sKeyIdxFileName() );
    if ( !File::exists(fp.fullPath()) )
    {
	BufferStringSet emptynms;
	if ( !setSelGrpSetNames(emptynms) )
	    return false;
    }

    SafeFileIO sfio( FilePath(basefp_,sKeyIdxFileName()).fullPath(), true );
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

   FilePath		basefp_;
};


static SGSelGrpManager* sgm = 0;


struct SGSelGrpManDeleter : public NamedObject
{ void doDel( CallBacker* ) { delete sgm; sgm = 0; } };

static SGSelGrpManager& SGM()
{
    if ( !sgm )
    {
	sgm = new SGSelGrpManager();
	static SGSelGrpManDeleter sgsmd;
	const_cast<SurveyInfo&>(SI()).deleteNotify(
		mCB(&sgsmd,SGSelGrpManDeleter,doDel) );
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
	nmfld_ = new uiGenInput( this, "Name" );
	nmfld_->attach( alignedBelow, listfld_ );
	nmfld_->setElemSzPol( uiObject::SmallMax );
	nmfld_->setStretch( 2, 0 );
    }

    infobut_ = new uiToolButton( this, "info", "Info",
	    			 mCB(this,uiSGSelGrp,showInfo) );
    infobut_->attach( rightTo, listfld_ );

    delbut_ = new uiToolButton( this, "trashcan", "Delete Selection-Groups",
	    		        mCB(this,uiSGSelGrp,delSelGrps) );
    delbut_->attach( alignedBelow, infobut_ );

    renamebut_ = new uiToolButton( this, "renameobj", "Rename Selection-Groups",
	    			   mCB(this,uiSGSelGrp,renameSelGrps) );
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
    BufferString info;
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
{
public:

uiRenameDlg( uiParent* p, const char* nm )
    : uiDialog(p,uiDialog::Setup("Rename Selection Group Set","","") )
{
    namefld_ = new uiGenInput( this, "Selection Group Set Name" );
    namefld_->setText( nm );
}

const char* getName()
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
	if ( mIsUdf(idx) || idx < 0 || !dlg.getName() )
	    return;
	SGM().renameSelGrpSet( listfld_->getText(), dlg.getName() ); 
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
    cleanupString( cleannm.buf(), false, false, false ); 
    return FilePath(SGM().basePath(),cleannm).fullPath();
}


SelGrpImporter::SelGrpImporter( const char* fnm )
{
    sd_ = StreamProvider( fnm ).makeIStream();
    if ( !sd_.usable() )
	{ errmsg_ = "Cannot open specified file"; return; }
}


SelGrpImporter::~SelGrpImporter()
{
    sd_.close();
}


ObjectSet<SelectionGrp> SelGrpImporter::getSelections()
{
    ObjectSet<SelectionGrp> selgrpset;
    if ( !sd_.usable() )
	return selgrpset;

    ascistream astrm( *sd_.istrm, true );

    if ( !astrm.isOfFileType(sKeyFileType) )
    {
	errmsg_ = "File type does not match with Cross-plot Selection";
	return selgrpset;
    }

    int nrselgrps = 0;
    IOPar par( astrm );
    
    if ( par.hasKey(sKeyNrSelGrps) )
	par.get( sKeyNrSelGrps, nrselgrps );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute(),"X")) )
	par.get( IOPar::compKey(sKey::Attribute(),"X"), xname_ );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute(),"Y")) )
	par.get( IOPar::compKey(sKey::Attribute(),"Y"), yname_ );
    if ( par.hasKey(IOPar::compKey(sKey::Attribute(),"Y2")) )
	par.get( IOPar::compKey(sKey::Attribute(),"Y2"), y2name_ );

    for ( int selidx=0; selidx < nrselgrps; selidx++ )
    {
	PtrMan<IOPar> selgrppar = par.subselect( selidx );
	if ( !selgrppar ) continue;

	SelectionGrp* selgrp = new SelectionGrp();
	selgrp->usePar( *selgrppar );
	selgrpset += selgrp;
    }

    sd_.close();

    return selgrpset;
}


SelGrpExporter::SelGrpExporter( const char* fnm )
{
    sd_ = StreamProvider( fnm ).makeOStream();
    if ( !sd_.usable() )
	{ errmsg_ = "Cannot open specified file to write"; return; }
}

SelGrpExporter::~SelGrpExporter()
{
    sd_.close();
}

bool SelGrpExporter::putSelections( const ObjectSet<SelectionGrp>& selgrps,
				    const char* xname, const char* yname,
				    const char* y2name )
{
    if ( !sd_.usable() ) return false;

    ascostream ostrm( *sd_.ostrm ); 
    std::ostream& strm = ostrm.stream();

    if ( !selgrps.size() )
    {
	errmsg_ = "No selections found";
	return false;
    }

    IOPar selectionpar;
    selectionpar.set( IOPar::compKey(sKey::Attribute(),"X"), xname );
    selectionpar.set( IOPar::compKey(sKey::Attribute(),"Y"), yname );
    if ( y2name )
	selectionpar.set( IOPar::compKey(sKey::Attribute(),"Y2"), y2name );
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

    selectionpar.write( ostrm.stream(), sKeyFileType );
    const bool ret = strm.good();
    if ( !ret )
	errmsg_ = "Error during write";
    sd_.close();
    return ret;
}


class uiSGSelDlg : public uiDialog
{
public:

uiSGSelDlg( uiParent* p, bool forread )
    : uiDialog(p,uiDialog::Setup("Select Cross-plot Selection Groups","",""))
    , forread_(forread)
{
    selgrp_ = new uiSGSelGrp( this, forread );
    selgrp_->selectionDone.notify( mCB(this,uiSGSelDlg,accept) );
}


bool acceptOK( CallBacker* )
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

const char* selGrSetNm() const 				{ return selgrpsetnm_; }
const BufferString& selGrpFileNm() const		{ return filenm_;}

const char* xName() const 				{ return xname_; }
const char* yName() const 				{ return yname_; }
const char* y2Name() const 				{ return y2name_; }

protected:

    ObjectSet<SelectionGrp>	selgrpset_;
    uiSGSelGrp* 		selgrp_;
    
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
    inpfld_ = new uiGenInput( this, "Cross-plot Selections" );
    selbut_ = new uiPushButton( this, "Select ..", mCB(this,uiSGSel,selectSGCB),
	    			false );
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
	cleanupString( cleannm.buf(), false, false, false );
	return FilePath(SGM().basePath(),cleannm).fullPath();
	//selgrpfilenm_ = FilePath(SGM().basePath(),cleannm).fullPath();
    }

    return selgrpfilenm_.buf();
}


uiReadSelGrp::uiReadSelGrp( uiParent* p, uiDataPointSetCrossPlotter& plotter )
    : uiDialog(p,uiDialog::Setup("Open Cross-plot Selection",mNoDlgTitle,
				"111.0.10"))
    , plotter_(plotter)
    , selgrpset_(plotter.selectionGrps())
    , y2selfld_(0)
{
    bool hasy2 = plotter.axisHandler(2);
    BufferStringSet nms;
    nms.add( plotter.axisHandler(0)->name() );
    nms.add( plotter.axisHandler(1)->name() );
    if ( hasy2 )
	nms.add( plotter.axisHandler(2)->name() );

    inpfld_ = new uiSGSel( this, true );
    inpfld_->selGrpSelected.notify( mCB(this,uiReadSelGrp,selectedCB) );

    uiLabeledComboBox* xselfld =
	new uiLabeledComboBox( this, plotter.axisHandler(0)->name() );
    xselfld_ = xselfld->box();
    xselfld->attach( centeredBelow, inpfld_ );
    xselfld_->display( false, false );
   
    uiLabeledComboBox* yselfld =
	new uiLabeledComboBox( this, plotter.axisHandler(1)->name() );
    yselfld_ = yselfld->box();
    yselfld->attach( alignedBelow, xselfld );
    yselfld_->display( false, false );
    
    ychkfld_ = new uiCheckBox( this, "Import Y1",
	    		       mCB(this,uiReadSelGrp,fldCheckedCB) );
    ychkfld_->attach( rightTo, yselfld );
    ychkfld_->display( false, false );
    
    if ( hasy2 )
    {
	uiLabeledComboBox* y2selfld =
	    new uiLabeledComboBox( this,plotter.axisHandler(2)->name() );
	y2selfld_ = y2selfld->box();
	y2selfld->attach( alignedBelow, yselfld );
	y2selfld_->display( false, false );
	y2chkfld_ = new uiCheckBox( this, "Import Y2",
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
	uiMSG().error( "Selected Selection-Group set is corrupted" );
	return;
    }

    selgrpset_ = inpfld_->selGrpSet();

    BufferStringSet axisnms;
    axisnms.add( plotter_.axisHandler(0)->name() );
    axisnms.add( plotter_.axisHandler(1)->name() );
    if ( plotter_.axisHandler(2) )
	axisnms.add( plotter_.axisHandler(2)->name() );

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
    axisnm.add( xselfld_->textOfItem(xselfld_->currentItem()) );
    if ( ychkfld_->isChecked() || !ychkfld_->isDisplayed() )
	axisnm.add( yselfld_->textOfItem(yselfld_->currentItem()) );
    if ( y2selfld_ && y2chkfld_->isChecked() )
	axisnm.add( y2selfld_->textOfItem(y2selfld_->currentItem()) );
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
	ltptval += rect.topLeft().x;
	ltptval += rect.topLeft().y;
	ltptval += altrect.topLeft().y;

	TypeSet<double> rbptval;
	rbptval += rect.bottomRight().x;
	rbptval += rect.bottomRight().y;
	rbptval += altrect.bottomRight().y;

	const bool onlyy2 = actselarea.axistype_ == SelectionArea::Y2;
	const int yaxisnr = (yaxis<0 || onlyy2) ? y2axis : yaxis;

	actselarea.worldrect_ =
	   uiWorldRect( ltptval[xaxis], ltptval[yaxisnr],
			rbptval[xaxis], rbptval[yaxisnr] );
	actselarea.worldrect_.checkCorners( true, false );
	if (hasalt && actselarea.axistype_==SelectionArea::Both)
	{
	   actselarea.altworldrect_ =
	       uiWorldRect( ltptval[xaxis], ltptval[y2axis],
			    rbptval[xaxis], rbptval[y2axis] );
	   actselarea.altworldrect_.checkCorners( true, false );
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
	   ptval += pts[idx].x; ptval += pts[idx].y;
	   ptval += altpts[idx].y;
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
	uiMSG().error( "Can't import selection group" );
	return false;
    }

    if ( xaxis==yaxis || yaxis==y2axis || y2axis==xaxis )
    {
	uiMSG().error( "Same parameter chosen for different axis" );
	return false;
    }

    if ( yaxis < 0 && !plotter_.axisHandler(2) )
    {
	uiMSG().error( "Choose axis properly" );
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
		actselarea.xaxisnm_ = plotter_.axisHandler(0)->name();
		actselarea.yaxisnm_ =
		    plotter_.axisHandler( yaxis < 0 ? 2 : 1 )->name();
		if ( hasalt && y2axis >=0 )
		    actselarea.altyaxisnm_ = plotter_.axisHandler(2)->name();
		newselgrp->addSelection( actselarea );
		selareaid++;
	    }
	    else
		selimportfailed = true;
	}

	delete selgrpset_.replace( selgrpset_.indexOf(selgrp), newselgrp );
    }

    if ( selimportfailed )
	uiMSG().message( "Some selectionareas could not be imported \n"
			 "as they fall outside the value ranges of the plot" );

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


bool uiReadSelGrp::acceptOK( CallBacker* )
{
    if ( !adjustSelectionGrps() )
	return false;
    plotter_.reDrawSelections();
    return true;
}


uiExpSelectionArea::uiExpSelectionArea( uiParent* p,
					const ObjectSet<SelectionGrp>& selgrps,
					uiExpSelectionArea::Setup su )
    : uiDialog(p,uiDialog::Setup("Save Selection Area",
				 "Specify parameters","111.0.9"))
    , selgrps_(selgrps)
    , setup_(su)
{
    outfld_ = new uiSGSel( this, false );
}


bool uiExpSelectionArea::acceptOK( CallBacker* )
{
    if ( !outfld_->isOK() )
    {
	uiMSG().error( "Please select an ouput name" );
	return false;
    }

    if ( File::exists(outfld_->selGrpFileNm()) )
    {
	if ( !uiMSG().askOverwrite("Selected selections already present, "
			      	   "Do you want to overwrite?") )
	    return false;
    }

    SGM().addSelGrpSet( outfld_->selGrpSetNm() );
    SelGrpExporter exp( outfld_->selGrpFileNm() );
    BufferString yaxisnm;
    if ( !exp.putSelections(selgrps_,setup_.xname_,
			    setup_.yname_,setup_.y2name_) )
	{ uiMSG().error(exp.errMsg()); return false; }

    return true;
}
