/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/


#include "uiseis2dfileman.h"
#include "uiseispsman.h"

#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "linesetposinfo.h"
#include "od_helpids.h"
#include "posinfo2dsurv.h"
#include "seis2ddata.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seis2dlinemerge.h"
#include "seiscube2linedata.h"
#include "stringbuilder.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
#include "timefun.h"
#include "zdomain.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uiimpexp2dgeom.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseis2dfrom3d.h"
#include "uiseisioobjinfo.h"
#include "uiseissampleeditor.h"
#include "uiseissel.h"
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
    , issidomain(ZDomain::isSI( ioobj.pars() ))
    , zistm((SI().zIsTime() && issidomain) || (!SI().zIsTime() && !issidomain))
{
    setCtrlStyle( CloseOnly );
    Survey::GMAdmin().updateGeometries( uiTaskRunnerProvider(this) );

    objinfo_ = new uiSeisIOObjInfo( this, ioobj );
    dataset_ = new Seis2DDataSet( ioobj );

    uiGroup* topgrp = new uiGroup( this, "Top" );
    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("2D lines"),
			 uiListBox::AboveMid );
    linefld_ = new uiListBox( topgrp, su );
    linefld_->selectionChanged.notify( mCB(this,uiSeis2DFileMan,lineSel) );

    linegrp_ = new uiManipButGrp( linefld_ );
    linegrp_->addButton( uiManipButGrp::Remove, uiStrings::phrRemove(
		    uiStrings::sLine()), mCB(this,uiSeis2DFileMan,removeLine) );
    linegrp_->addButton( "mergelines", uiStrings::phrMerge(
			uiStrings::sLine(mPlural)),mCB(this,uiSeis2DFileMan,
			mergeLines) );
    linegrp_->addButton( "browseseis", tr("Browse/Edit this line"),
			mCB(this,uiSeis2DFileMan,browsePush) );
    if ( SI().has3D() )
	linegrp_->addButton( "extr3dinto2d", tr("Extract from 3D cube"),
			mCB(this,uiSeis2DFileMan,extrFrom3D) );
    linegrp_->attach( rightOf, linefld_->box() );

    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", OD::Horizontal );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    fillLineBox();

    mTriggerInstanceCreatedNotifier();
    lineSel(0);
}


uiSeis2DFileMan::~uiSeis2DFileMan()
{
    delete objinfo_;
    delete dataset_;
}


void uiSeis2DFileMan::fillLineBox()
{
    uiListBox* lb = linefld_;
    const int curitm = lb->size() ? lb->currentItem() : 0;
    BufferStringSet linenames;
    SeisIOObjInfo::Opts2D opts2d; opts2d.zdomky_ = "*";
    objinfo_->getLineNames( linenames, opts2d );
    lb->setEmpty();
    lb->addItems( linenames );
    lb->setCurrentItem( curitm );
}


void uiSeis2DFileMan::lineSel( CallBacker* )
{
    infofld_->setText( "" );
    BufferStringSet linenms;
    linefld_->getChosen( linenms );
    StringBuilder sb;
    for ( int idx=0; idx<linenms.size(); idx++ )
    {
	const auto geomid = SurvGeom::getGeomID( linenms.get(idx) );
	const int lineidx = dataset_->indexOf( geomid );
	if ( lineidx < 0 )
	    { pErrMsg("Huh"); continue; }

	BufferString displinenm( linenms.get(idx) );
	displinenm.add( " [ID=" ).add( geomid.getI() ).add( "]" );

	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	const bool hasrg = dataset_->getRanges( geomid, trcrg, zrg );

	PosInfo::Line2DData l2dd;
	const auto& geom2d = SurvGeom::get2D( geomid );
	l2dd = geom2d.data();
	if ( l2dd.isEmpty() )
	{
	    sb.add( "\nAbsent or empty geometry for line " );
	    sb.add( displinenm );
	    continue;
	}

	const int sz = trcrg.nrSteps() + 1;
	PosInfo::Line2DPos firstpos, lastpos;
	l2dd.getPos( trcrg.start, firstpos );
	l2dd.getPos( trcrg.stop, lastpos );

	if ( !hasrg )
	    sb.add( "\nCannot read ranges for line: " ).add( displinenm )
		.add( "\nCBVS file may be corrupt or missing.\n" );
	else
	{
	    if ( idx > 0 )
		sb.add( "\n\n" );
	    sb.add( "Line: " ).add( displinenm );
	    sb.add( "\nNumber of traces: " ).add( sz )
		.add( "\nFirst trace: " );
	    if ( l2dd.getPos(trcrg.start,firstpos) )
		sb.add( firstpos.nr_ )
		   .add( " " ).add( firstpos.coord_.toString() );
	    sb.add( "\nLast trace: " );
	    if ( l2dd.getPos(trcrg.stop,lastpos) )
		sb.add( lastpos.nr_ )
		   .add( " " ).add( lastpos.coord_.toString() );

#define mAddZRangeTxt(memb) sb.add( zistm ? mNINT32(1000*memb) : memb )
	    sb.add( "\nZ-range: " ); mAddZRangeTxt(zrg.start); sb.add( " - " );
	    mAddZRangeTxt(zrg.stop);
	    sb.add( " [" ); mAddZRangeTxt(zrg.step); sb.add( "]" );
	}

	const IOObj& ioobj = *objinfo_->ioObj();
	SeisIOObjInfo sobinf( ioobj );
	const int nrcomp = sobinf.nrComponents( geomid );
	if ( nrcomp > 1 )
	    sb.add( "\nNumber of components: " ).add( nrcomp );

	BufferString fname = SeisCBVS2DLineIOProvider::getFileName( ioobj,
								    geomid );
	File::Path fp( fname );

	sb.add( "\nLocation: " ).add( fp.pathOnly() )
	    .add( "\nFile name: " ).add( fp.fileName() )
	    .add( "\nFile size: " ).add( File::getFileSizeString( fname ) );
	const BufferString timestr = Time::getUsrFileDateTime( fname );
	if ( !timestr.isEmpty() )
	    { sb.add( "\nLast modified: " ).add( timestr ); }
    }

    infofld_->setText( sb.result() );
}


void uiSeis2DFileMan::removeLine( CallBacker* )
{
    BufferStringSet sellines;
    linefld_->getChosen( sellines );
    if ( sellines.isEmpty() ||
	!uiMSG().askRemove(tr("All selected lines "
	    "will be removed.\n\nDo you want to continue?")) )
	return;

    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const char* linenm = sellines.get(idx);
	dataset_->remove( SurvGeom::getGeomID(linenm) );
	linefld_->removeItem( linenm );
    }

}


void uiSeis2DFileMan::browsePush( CallBacker* )
{
    if ( !objinfo_ || !objinfo_->ioObj() )
	return;

    const auto geomid = SurvGeom::getGeomID( linefld_->getText() );
    uiSeisSampleEditor::launch( this, objinfo_->ioObj()->key(), geomid );
    fillLineBox();
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
    BufferStringSet lnms; objinf_.getLineNames( lnms );
    uiLabeledComboBox* lcb1 =
	new uiLabeledComboBox( geomgrp, lnms, tr("First line") );
    uiLabeledComboBox* lcb2 = new uiLabeledComboBox( geomgrp, lnms,
							    uiStrings::sAdd() );
    lcb2->attach( alignedBelow, lcb1 );
    ln1fld_ = lcb1->box(); ln2fld_ = lcb2->box();
    ln1fld_->setCurrentItem( sellns.get(0) );
    ln2fld_->setCurrentItem( sellns.get(1) );
    ln1fld_->selectionChanged.notify(
	    mCB(this,uiSeis2DFileManMergeDlg,fillData2MergeCB) );
    ln2fld_->selectionChanged.notify(
	    mCB(this,uiSeis2DFileManMergeDlg,fillData2MergeCB) );


    const char* mrgopts[]
	= { "Match trace numbers", "Match coordinates", "Bluntly append", 0 };
    mrgoptfld_ = new uiGenInput( geomgrp, tr("Merge method"),
				 StringListInpSpec(mrgopts) );
    mrgoptfld_->attach( alignedBelow, lcb2 );
    mrgoptfld_->valuechanged.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );


    renumbfld_ = new uiGenInput( geomgrp, tr("Renumber; Start/step numbers"),
				 IntInpSpec(1), IntInpSpec(1) );
    renumbfld_->setWithCheck( true );
    renumbfld_->setChecked( true );
    renumbfld_->attach( alignedBelow, mrgoptfld_ );

    double defsd = SI().crlDistance() / 2;
    if ( SI().xyInFeet() ) defsd *= mToFeetFactorD;
    snapdistfld_ =
	new uiGenInput( geomgrp, tr("Snap distance"), DoubleInpSpec(defsd));
    snapdistfld_->attach( alignedBelow, renumbfld_ );

    outfld_ = new uiGenInput( geomgrp, tr("New line name"), StringInpSpec() );
    outfld_->attach( alignedBelow, snapdistfld_ );

    uiSeparator* horsep = new uiSeparator( this, "", OD::Vertical );
    horsep->attach( stretchedRightTo, geomgrp );
    uiGroup* datagrp = new uiGroup( this );
    stckfld_ = new uiGenInput( datagrp, tr("Duplicate positions"),
	       BoolInpSpec(true,uiStrings::sStack(),uiStrings::sUseFirst()) );
    datagrp->setHAlignObj( stckfld_->attachObj() );
    datagrp->attach( rightTo, geomgrp );
    datagrp->attach( ensureRightOf, horsep );

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Data Sets to merge") );
    data2mergefld_ = new uiListBox( datagrp, su );
    data2mergefld_->attach( alignedBelow, stckfld_ );


    postFinalise().notify( mCB(this,uiSeis2DFileManMergeDlg,initWin) );
}

void initWin( CallBacker* )
{
    optSel(0);
    fillData2MergeCB( 0 );
    renumbfld_->valuechanged.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );
    renumbfld_->checked.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );
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
bool acceptOK()
{
    const char* outnm = outfld_->text();
    if ( !outnm || !*outnm )
	mErrRet( uiStrings::phrEnter(tr("a name for the merged line")) );

    Pos::GeomID outgeomid = Geom2DImpHandler::getGeomID( outnm );
    if ( mIsUdfGeomID(outgeomid) )
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
	    mErrRet( uiStrings::phrSpecify(tr("a valid snap distance")) );
    }

    uiTaskRunner taskrun( this );
    bool rettype = false;
    if ( TaskRunner::execute(&taskrun,lmrgr) )
	rettype = uiMSG().askGoOn( tr("Merge successfully completed"),
				   tr("Done with merging"),
				   tr("Want to merge more lines") );
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
    const DBKey lsid( objinfo_->ioObj()->key() );
    delete objinfo_;
    objinfo_ = new uiSeisIOObjInfo( this, lsid );
    if ( objinfo_->isOK() )
    {
	delete dataset_;
	dataset_ = new Seis2DDataSet( *(objinfo_->ioObj()) );
    }
    fillLineBox();
}


void uiSeis2DFileMan::mergeLines( CallBacker* )
{
    if ( linefld_->size() < 2 ) return;

    BufferStringSet sellnms; int firstsel = -1;
    for ( int idx=0; idx<linefld_->size(); idx++ )
    {
	if ( linefld_->isChosen(idx) )
	{
	    sellnms.add( linefld_->itemText(idx) );
	    if ( firstsel < 0 ) firstsel = idx;
	    if ( sellnms.size() > 1 ) break;
	}
    }
    if ( firstsel < 0 )
	{ firstsel = 0; sellnms.add( linefld_->itemText(0) ); }
    if ( sellnms.size() == 1 )
    {
	if ( firstsel >= linefld_->size() )
	    firstsel = -1;
	sellnms.add( linefld_->itemText(firstsel+1) );
    }

    uiSeis2DFileManMergeDlg dlg( this, *objinfo_, sellnms );
    if ( dlg.go() )
	redoAllLists();
}


void uiSeis2DFileMan::extrFrom3D( CallBacker* )
{
    uiSeis2DFrom3D dlg( this );
    if ( dlg.go() )
	redoAllLists();
}
