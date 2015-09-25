/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseis2dfileman.h"
#include "uiseispsman.h"

#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "seis2ddata.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seis2dlinemerge.h"
#include "seiscube2linedata.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "zdomain.h"
#include "linesetposinfo.h"

#include "uitoolbutton.h"
#include "uigeninputdlg.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseis2dfrom3d.h"
#include "uiseisioobjinfo.h"
#include "uiseisbrowser.h"
#include "uiseissel.h"
#include "uisplitter.h"
#include "uiseparator.h"
#include "uitextedit.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiSeis2DFileMan)


uiSeis2DFileMan::uiSeis2DFileMan( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrManage( tr("2D Seismic Lines")),
                                 mNoDlgTitle,
				 mODHelpKey(mSeis2DManHelpID) ))
    , issidomain(ZDomain::isSI( ioobj.pars() ))
    , zistm((SI().zIsTime() && issidomain) || (!SI().zIsTime() && !issidomain))
{
    setCtrlStyle( CloseOnly );
    Survey::GMAdmin().updateGeometries( 0 );

    objinfo_ = new uiSeisIOObjInfo( ioobj );
    dataset_ = new Seis2DDataSet( ioobj );

    uiGroup* topgrp = new uiGroup( this, "Top" );
    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("2D lines"),
			 uiListBox::AboveMid );
    linefld_ = new uiListBox( topgrp, su );
    linefld_->selectionChanged.notify( mCB(this,uiSeis2DFileMan,lineSel) );

    linegrp_ = new uiManipButGrp( linefld_ );
    linegrp_->addButton( uiManipButGrp::Remove, "Remove line",
			mCB(this,uiSeis2DFileMan,removeLine) );
    linegrp_->addButton( "mergelines", "Merge lines",
			mCB(this,uiSeis2DFileMan,mergeLines) );
    linegrp_->addButton( "browseseis", "Browse/edit this line",
		        mCB(this,uiSeis2DFileMan,browsePush) );
    if ( SI().has3D() )
	linegrp_->addButton( "extr3dinto2d", "Extract from 3D cube",
			mCB(this,uiSeis2DFileMan,extrFrom3D) );
    linegrp_->attach( rightOf, linefld_->box() );

    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
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
    BufferString txt;
    for ( int idx=0; idx<linenms.size(); idx++ )
    {
	const Pos::GeomID geomid = Survey::GM().getGeomID( linenms.get(idx) );
	const int lineidx = dataset_->indexOf( geomid );
	if ( lineidx < 0 ) { pErrMsg("Huh"); continue; }

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
	    txt += ( "\nCannot find geometry for line: " );
	    txt += linenms.get(idx);
	    continue;
	}

	const int sz = trcrg.nrSteps() + 1;
	PosInfo::Line2DPos firstpos, lastpos;
	l2dd.getPos( trcrg.start, firstpos );
	l2dd.getPos( trcrg.stop, lastpos );

	if ( hasrg )
	{
	    if ( idx > 0 )
		txt += "\n\n";
	    txt += "Details for line: "; txt += linenms.get(idx);
	    txt += "\nNumber of traces: "; txt += sz;
	    txt += "\nFirst trace: ";
	    if ( l2dd.getPos(trcrg.start,firstpos) )
		txt.add( firstpos.nr_ )
		   .add( " " ).add( firstpos.coord_.toString() );
	    txt += "\nLast trace: ";
	    if ( l2dd.getPos(trcrg.stop,lastpos) )
		txt.add( lastpos.nr_ )
		   .add( " " ).add( lastpos.coord_.toString() );

#define mAddZRangeTxt(memb) txt += zistm ? mNINT32(1000*memb) : memb
	    txt += "\nZ-range: "; mAddZRangeTxt(zrg.start); txt += " - ";
	    mAddZRangeTxt(zrg.stop);
	    txt += " ["; mAddZRangeTxt(zrg.step); txt += "]";
	}
	else
	{
	    txt += "\nCannot read ranges for line: "; txt += linenms.get(idx);
	    txt += "\nCBVS file might be corrupt or missing.\n";
	}

	const IOObj& ioobj = *objinfo_->ioObj();
	SeisIOObjInfo sobinf( ioobj );
	const int nrcomp = sobinf.nrComponents( geomid );
	if ( nrcomp > 1 )
	    { txt += "\nNumber of components: "; txt += nrcomp; }

	BufferString fname = SeisCBVS2DLineIOProvider::getFileName( ioobj,
								    geomid );
	FilePath fp( fname );

	txt += "\nLocation: "; txt += fp.pathOnly();
	txt += "\nFile name: "; txt += fp.fileName();
	txt += "\nFile size: ";
	txt += File::getFileSizeString( fname );
	const char* timestr = File::timeLastModified( fname );
	if ( timestr ) { txt += "\nLast modified: "; txt += timestr; }
    }

    infofld_->setText( txt );
}


void uiSeis2DFileMan::removeLine( CallBacker* )
{
    BufferStringSet sellines;
    linefld_->getChosen( sellines );
    if ( sellines.isEmpty() ||
	!uiMSG().askRemove(tr("All selected lines "
	    "will be removed. Do you want to continue?")) )
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
    if ( !objinfo_ || !objinfo_->ioObj() ) return;

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
    ln1fld_->setCurrentItem( sellns.get(0) );
    ln2fld_->setCurrentItem( sellns.get(1) );
    ln1fld_->selectionChanged.notify(
	    mCB(this,uiSeis2DFileManMergeDlg,fillData2MergeCB) );
    ln2fld_->selectionChanged.notify(
	    mCB(this,uiSeis2DFileManMergeDlg,fillData2MergeCB) );


    const char* mrgopts[]
	= { "Match trace numbers", "Match coordinates", "Bluntly append", 0 };
    mrgoptfld_ = new uiGenInput( geomgrp, "Merge method",
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
			       BoolInpSpec(true,tr("Stack"),tr("Use first")) );
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
bool acceptOK( CallBacker* )
{
    const char* outnm = outfld_->text();
    if ( !outnm || !*outnm )
	mErrRet( tr("Please enter a name for the merged line") );

    Pos::GeomID outgeomid = Survey::GM().getGeomID( outnm );
    if ( outgeomid != Survey::GeometryManager::cUndefGeomID() )
    {
	 uiString msg = tr("The 2D Line '%1' already exists. If you overwrite "
			   "its geometry, all the associated data will be "
			   "affected. Do you still want to overwrite?")
				.arg( outnm );
	 if ( !uiMSG().askOverwrite(msg) )
	     return false;
	 mDynamicCastGet( Survey::Geometry2D*, geom2d,
			  Survey::GMAdmin().getGeometry(outgeomid) );
	 if ( !geom2d )
	     return false;

	 geom2d->dataAdmin().setEmpty();
	 geom2d->touch();
    }
    else
    {
	Survey::Geometry2D* newgeom =
	    new Survey::Geometry2D( new PosInfo::Line2DData );
	newgeom->dataAdmin().setLineName( outnm );
	uiString msg;
	outgeomid = Survey::GMAdmin().addNewEntry( newgeom, msg );
	if ( outgeomid == Survey::GeometryManager::cUndefGeomID() )
	    mErrRet( msg );
    }

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
	lmrgr.snapdist_ = snapdistfld_->getdValue();
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
    if ( linefld_->size() < 2 ) return;

    BufferStringSet sellnms; int firstsel = -1;
    for ( int idx=0; idx<linefld_->size(); idx++ )
    {
	if ( linefld_->isChosen(idx) )
	{
	    sellnms.add( linefld_->textOfItem(idx) );
	    if ( firstsel < 0 ) firstsel = idx;
	    if ( sellnms.size() > 1 ) break;
	}
    }
    if ( firstsel < 0 )
	{ firstsel = 0; sellnms.add( linefld_->textOfItem(0) ); }
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


void uiSeis2DFileMan::extrFrom3D( CallBacker* )
{
    uiSeis2DFrom3D dlg( this );
    if ( dlg.go() )
	redoAllLists();
}
