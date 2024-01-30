/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseis2dfileman.h"

#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "od_helpids.h"
#include "posinfo2d.h"
#include "seis2ddata.h"
#include "seis2dlinemerge.h"
#include "seiscbvs2d.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "zdomain.h"

#include "ui2dgeomman.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseisbrowser.h"
#include "uiseisioobjinfo.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

mDefineInstanceCreatedNotifierAccess(uiSeis2DFileMan)


uiSeis2DFileMan::uiSeis2DFileMan( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrManage( tr("2D Seismic Lines")),
                                 mNoDlgTitle,
				 mODHelpKey(mSeis2DManHelpID) ))
    , issidomain(true) // deprecated
    , zistm(true)   // deprecated
{
    setCtrlStyle( CloseOnly );
    Survey::GMAdmin().updateGeometries( nullptr );

    objinfo_ = new uiSeisIOObjInfo( ioobj );
    dataset_ = new Seis2DDataSet( ioobj );

    auto* topgrp = new uiGroup( this, "Top" );
    const uiListBox::Setup su( OD::ChooseAtLeastOne, tr("2D lines"),
			       uiListBox::AboveMid );
    linefld_ = new uiListBox( topgrp, su );
    mAttachCB( linefld_->selectionChanged, uiSeis2DFileMan::lineSel );

    linegrp_ = new uiManipButGrp( linefld_ );
    linegrp_->addButton( uiManipButGrp::Remove,
			 uiStrings::phrDelete(tr("data for line")),
			 mCB(this,uiSeis2DFileMan,removeLine) );
    linegrp_->addButton( "mergelines", uiStrings::phrMerge(
			uiStrings::sLine(mPlural)),mCB(this,uiSeis2DFileMan,
			mergeLines) );
    linegrp_->addButton( "browseseis", tr("Browse/edit this line"),
		        mCB(this,uiSeis2DFileMan,browsePush) );
    linegrp_->attach( rightOf, linefld_->box() );

    auto* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 10 );
    infofld_->setPrefWidthInChar( 75 );

    auto* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    fillLineBox();

    mTriggerInstanceCreatedNotifier();
    mAttachCB( postFinalize(), uiSeis2DFileMan::lineSel );
}


uiSeis2DFileMan::~uiSeis2DFileMan()
{
    detachAllNotifiers();
    delete objinfo_;
    delete dataset_;
}


void uiSeis2DFileMan::fillLineBox()
{
    uiListBox* lb = linefld_;
    const int curitm = lb->size() ? lb->currentItem() : 0;
    BufferStringSet linenames;
    SeisIOObjInfo::Opts2D opts2d; opts2d.zdomky_ = "*";
    objinfo_->ioObjInfo().getLineNames( linenames, opts2d );
    lb->setEmpty();
    lb->addItems( linenames );
    lb->setCurrentItem( curitm );
}


void uiSeis2DFileMan::lineSel( CallBacker* )
{
    infofld_->setText( "" );
    BufferStringSet linenms;
    linefld_->getChosen( linenms );
    uiString txt;
    for ( int idx=0; idx<linenms.size(); idx++ )
    {
	const Pos::GeomID geomid = Survey::GM().getGeomID( linenms.get(idx) );
	const int lineidx = dataset_->indexOf( geomid );
	if ( lineidx < 0 )
	{
	    pErrMsg("Huh");
	    continue;
	}

	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	const bool hasrg = dataset_->getRanges( geomid, trcrg, zrg );

	PosInfo::Line2DData l2dd;
	const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
	mDynamicCastGet( const Survey::Geometry2D*, geom2d, geom )
	if ( geom2d )
	    l2dd = geom2d->data();
	if ( !geom2d || l2dd.isEmpty() )
	{
	    txt.append( tr("Cannot find geometry for line: %1").
						    arg(linenms.get(idx)) );
	    txt.addNewLine();
	    continue;
	}

	const int sz = trcrg.nrSteps() + 1;
	PosInfo::Line2DPos firstpos, lastpos;
	l2dd.getPos( trcrg.start, firstpos );
	l2dd.getPos( trcrg.stop, lastpos );

	if ( hasrg )
	{
	    if ( idx > 0 )
		txt.addNewLine();

	    txt.appendPhrase( tr("Details for line: %1").arg(linenms.get(idx)),
			      uiString::NoSep, uiString::OnSameLine );
	    txt.addNewLine();
	    txt.append( tr("Number of traces: %1").arg(sz) );
	    txt.addNewLine();
	    txt.append( tr("First trace: %1 %2") );
	    const int nrxydec = SI().nrXYDecimals();
	    if ( l2dd.getPos(trcrg.start,firstpos) )
		txt.arg( firstpos.nr_ )
		   .arg( firstpos.coord_.toPrettyString(nrxydec) ).addNewLine();
	    txt.append( tr("Last trace: %1 %2") );
	    if ( l2dd.getPos(trcrg.stop,lastpos) )
		txt.arg( lastpos.nr_ )
		   .arg( lastpos.coord_.toPrettyString(nrxydec) ).addNewLine();

	    const ZDomain::Info& zinfo = objinfo_->zDomain();
	    const int nrzdec = zinfo.def_.nrZDecimals( zrg.step );
	    zrg.scale( zinfo.def_.userFactor() );
	    uiString rangestr = zinfo.def_.getRange();
	    const uiString unitstr = zinfo.uiUnitStr_( true );
	    if ( !unitstr.isEmpty() )
		rangestr.withUnit( unitstr );
	    const uiString rgstr = tr("%1: %2 - %3 [%4]")
				.arg( rangestr )
				.arg( toString(zrg.start,nrzdec) )
				.arg( toString(zrg.stop,nrzdec) )
				.arg( toString(zrg.step,nrzdec) );
	    txt.append( rgstr );
	}
	else
	{
	    txt.append( tr("Cannot read ranges for line: %1")
						    .arg(linenms.get(idx)) );
	    txt.addNewLine();
	    txt.append( tr("CBVS file might be corrupt or missing.") );
	    txt.addNewLine();
	}

	const IOObj& ioobj = *objinfo_->ioObj();
	SeisIOObjInfo sobinf( ioobj );
	const int nrcomp = sobinf.nrComponents( geomid );
	if ( nrcomp > 1 )
	{
	    txt.addNewLine();
	    txt.append( tr("Number of components: %1").arg(nrcomp) );
	}

	BufferString fname = SeisCBVS2DLineIOProvider::getFileName( ioobj,
								    geomid );
	FilePath fp( fname );
	txt.addNewLine();
	txt.append( tr("Location: %1").arg(fp.pathOnly()) );
	txt.addNewLine();
	txt.append( tr("File name: %1").arg(fp.fileName()) );
	txt.addNewLine();
	txt.append( tr("File size: %1").arg(
				    File::getFileSizeString(fname.buf())) );
	StringView timestr( File::timeLastModified(fname) );
	if ( !timestr.isEmpty() )
	{
	    txt.addNewLine();
	    txt.append( tr("Last modified: %1").arg(timestr) );
	}

	txt.addNewLine();
    }

    infofld_->setText( txt );
}


void uiSeis2DFileMan::removeLine( CallBacker* )
{
    BufferStringSet sellines;
    linefld_->getChosen( sellines );
    if ( sellines.isEmpty() ||
	!uiMSG().askDelete(tr("Data for the selected lines "
	    "will be deleted from the dataset '%1'.\n\n"
	    "Do you want to continue?").arg(dataset_->name())) )
	return;

    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const char* linenm = sellines.get(idx);
        dataset_->remove( Survey::GM().getGeomID(linenm) );
	linefld_->removeItem( linenm );
    }

}


void uiSeis2DFileMan::browsePush( CallBacker* )
{
    if ( !objinfo_ || !objinfo_->ioObj() )
	return;

    const LineKey lk( linefld_->getText() );
    uiSeisBrowser::doBrowse( this, *objinfo_->ioObj(), true, &lk );
}


class uiSeis2DFileManMergeDlg : public uiDialog
{ mODTextTranslationClass(uiSeis2DFileManMergeDlg);
public:

uiSeis2DFileManMergeDlg( uiParent* p, const uiSeisIOObjInfo& objinf,
			 const BufferStringSet& sellns )
    : uiDialog(p,Setup(tr("Merge lines"),tr("Merge two lines into a new one"),
		       mODHelpKey(mSeis2DFileManMergeDlgHelpID) ) )
    , objinf_(objinf)
{
    uiGroup* geomgrp = new uiGroup( this );
    BufferStringSet lnms; objinf_.ioObjInfo().getLineNames( lnms );
    uiLabeledComboBox* lcb1 =
	new uiLabeledComboBox( geomgrp, lnms, tr("First line") );
    uiLabeledComboBox* lcb2 = new uiLabeledComboBox( geomgrp, lnms,
						     uiStrings::sAdd() );
    lcb2->attach( alignedBelow, lcb1 );
    ln1fld_ = lcb1->box(); ln2fld_ = lcb2->box();
    ln1fld_->setCurrentItem( sellns.get(0).buf() );
    ln2fld_->setCurrentItem( sellns.get(1).buf() );
    mAttachCB( ln1fld_->selectionChanged,
				    uiSeis2DFileManMergeDlg::fillData2MergeCB );
    mAttachCB( ln2fld_->selectionChanged,
				    uiSeis2DFileManMergeDlg::fillData2MergeCB );

    uiStringSet mrgopts;
    mrgopts.add( tr("Match trace numbers") );
    mrgopts.add( tr("Match coordinates") );
    mrgopts.add( tr("Bluntly append") );
    mrgoptfld_ = new uiGenInput( geomgrp, uiStrings::phrMerge(tr("method")),
				 StringListInpSpec(mrgopts) );
    mrgoptfld_->attach( alignedBelow, lcb2 );
    mAttachCB( mrgoptfld_->valuechanged, uiSeis2DFileManMergeDlg::optSel );

    renumbfld_ = new uiGenInput( geomgrp, tr("Renumber; Start/step numbers"),
				 IntInpSpec(1), IntInpSpec(1) );
    renumbfld_->setWithCheck( true );
    renumbfld_->setChecked( true );
    renumbfld_->attach( alignedBelow, mrgoptfld_ );

    const double defsd = SI().crlDistance() / 2;
    snapdistfld_ =
	new uiGenInput( geomgrp, tr("Snap distance"), DoubleInpSpec(defsd));
    snapdistfld_->attach( alignedBelow, renumbfld_ );

    outfld_ = new uiGenInput( geomgrp, tr("New line name"), StringInpSpec() );
    outfld_->attach( alignedBelow, snapdistfld_ );

    uiSeparator* horsep = new uiSeparator( this, "", OD::Vertical );
    horsep->attach( stretchedRightTo, geomgrp );
    uiGroup* datagrp = new uiGroup( this );
    stckfld_ = new uiGenInput( datagrp, tr("Duplicate positions"),
			       BoolInpSpec(true,tr("Stack"),tr("Use first")) );
    datagrp->setHAlignObj( stckfld_->attachObj() );
    datagrp->attach( rightTo, geomgrp );
    datagrp->attach( ensureRightOf, horsep );

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Data Sets to merge") );
    data2mergefld_ = new uiListBox( datagrp, su );
    data2mergefld_->attach( alignedBelow, stckfld_ );

    mAttachCB( postFinalize(), uiSeis2DFileManMergeDlg::initWin );
}


~uiSeis2DFileManMergeDlg()
{
    detachAllNotifiers();
}


void initWin( CallBacker* )
{
    optSel( nullptr);
    fillData2MergeCB( nullptr );
    mAttachCB( renumbfld_->valuechanged, uiSeis2DFileManMergeDlg::optSel );
    mAttachCB( renumbfld_->checked, uiSeis2DFileManMergeDlg::optSel );
}


void fillData2MergeCB( CallBacker* )
{
    BufferStringSet l1dataset, l2dataset;
    Seis2DDataSet::getDataSetsOnLine( ln1fld_->text(), l1dataset );
    Seis2DDataSet::getDataSetsOnLine( ln2fld_->text(), l2dataset );
    BufferStringSet commondataset;
    for ( int l1idx=0; l1idx<l1dataset.size(); l1idx++ )
    {
	const char* l1nm = l1dataset.get(l1idx).buf();
	if ( l2dataset.isPresent(l1nm) )
	    commondataset.add( l1nm );
    }

    data2mergefld_->setEmpty();
    data2mergefld_->addItems( commondataset );
}


void optSel( CallBacker* )
{
    const int opt = mrgoptfld_->getIntValue();
    stckfld_->display( opt < 2 );
    renumbfld_->display( opt > 0 );
    snapdistfld_->display( opt == 1 );
}

#define mErrRet(s) { uiMSG().error(s); return false; }
bool acceptOK( CallBacker* ) override
{
    StringView outnm = outfld_->text();
    if ( outnm.isEmpty() )
	mErrRet( tr("Please enter a name for the merged line") );

    Pos::GeomID outgeomid = Geom2DImpHandler::getGeomID( outnm );
    if ( outgeomid == mUdfGeomID )
	return false;

    BufferStringSet seldatanms;
    data2mergefld_->getChosen( seldatanms );
    if ( seldatanms.isEmpty() )
	mErrRet( tr("No datas chosen to merge, please select a data.") );

    Seis2DLineMerger lmrgr( seldatanms, outgeomid );
    lmrgr.lnm1_ = ln1fld_->text();
    lmrgr.lnm2_ = ln2fld_->text();
    if ( lmrgr.lnm1_ == lmrgr.lnm2_ )
	mErrRet( tr("Respectfully refusing to merge a line with itself") );

    lmrgr.outlnm_ = outnm;
    lmrgr.opt_ = (Seis2DLineMerger::Opt)mrgoptfld_->getIntValue();
    lmrgr.stckdupl_ = stckfld_->getBoolValue();
    lmrgr.renumber_ = lmrgr.opt_ != Seis2DLineMerger::MatchTrcNr
		   && renumbfld_->isChecked();

    if ( lmrgr.renumber_ )
    {
	lmrgr.numbering_.start = renumbfld_->getIntValue(0);
	lmrgr.numbering_.step = renumbfld_->getIntValue(1);
    }

    if ( lmrgr.opt_ == Seis2DLineMerger::MatchCoords )
    {
	lmrgr.snapdist_ = snapdistfld_->getDValue();
	if ( mIsUdf(lmrgr.snapdist_) || lmrgr.snapdist_ < 0 )
	    mErrRet( tr("Please specify a valid snap distance") );
    }

    uiTaskRunner taskrun( this );
    bool rettype = false;
    if ( TaskRunner::execute(&taskrun,lmrgr) )
	rettype = uiMSG().askGoOn( tr("Merge successfully completed."),
				   tr( "Done with merging"),
				   tr( "Want to merge more lines") );
    return rettype;
}

    const uiSeisIOObjInfo&	objinf_;

    uiComboBox*			ln1fld_;
    uiComboBox*			ln2fld_;
    uiListBox*			data2mergefld_;
    uiGenInput*			mrgoptfld_;
    uiGenInput*			stckfld_;
    uiGenInput*			renumbfld_;
    uiGenInput*			snapdistfld_;
    uiGenInput*			outfld_;

};


void uiSeis2DFileMan::redoAllLists()
{
    const MultiID lsid( objinfo_->ioObj()->key() );
    delete objinfo_;
    objinfo_ = new uiSeisIOObjInfo( lsid );
    if ( objinfo_->isOK() )
    {
	delete dataset_;
	dataset_ = new Seis2DDataSet( *(objinfo_->ioObj()) );
    }
    fillLineBox();
}


void uiSeis2DFileMan::mergeLines( CallBacker* )
{
    if ( linefld_->size() < 2 )
	return;

    BufferStringSet sellnms; int firstsel = -1;
    for ( int idx=0; idx<linefld_->size(); idx++ )
    {
	if ( linefld_->isChosen(idx) )
	{
	    sellnms.add( linefld_->textOfItem(idx) );
	    if ( firstsel < 0 ) firstsel = idx;
	    if ( sellnms.size() > 1 )
		break;
	}
    }

    if ( firstsel < 0 )
    {
	firstsel = 0;
	sellnms.add( linefld_->textOfItem(0) );
    }

    if ( sellnms.size() == 1 )
    {
	if ( firstsel >= linefld_->size() )
	    firstsel = -1;

	sellnms.add( linefld_->textOfItem(firstsel+1) );
    }

    uiSeis2DFileManMergeDlg dlg( this, *objinfo_, sellnms );
    if ( dlg.go() )
	redoAllLists();
}
