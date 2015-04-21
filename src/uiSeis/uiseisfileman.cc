/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseisfileman.h"

#include "cbvsreadmgr.h"
#include "cubesampling.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "seis2dlineio.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "segydirecttr.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "zdomain.h"
#include "separstr.h"
#include "timedepthconv.h"

#include "ui2dgeomman.h"
#include "uitoolbutton.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uimergeseis.h"
#include "uisegydefdlg.h"
#include "uiseispsman.h"
#include "uiseisbrowser.h"
#include "uiseiscopy.h"
#include "uiseisioobjinfo.h"
#include "uiseis2dfileman.h"
#include "uiseis2dgeom.h"
#include "uisplitter.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiSeisFileMan)

#define mCapt is2d ? "Manage 2D Seismics" : "Manage 3D Seismics"
#define mHelpID is2d ? mODHelpKey(mSeisFileMan2DHelpID) : \
                       mODHelpKey(mSeisFileMan3DHelpID)
uiSeisFileMan::uiSeisFileMan( uiParent* p, bool is2d )
    :uiObjFileMan(p,uiDialog::Setup(mCapt,mNoDlgTitle,mHelpID)
				   .nrstatusflds(1).modal(false),
		   SeisTrcTranslatorGroup::ioContext())
    , is2d_(is2d)
    , browsebut_(0)
    , man2dlines_(0)
    , dumpgeombut_(0)
    , mergecubesbut_(0)
{
    IOObjContext* freshctxt = Seis::getIOObjContext(
					is2d_ ? Seis::Line : Seis::Vol, true );
    ctxt_ = *freshctxt;
    delete freshctxt;

    createDefaultUI( true );
    selgrp_->getListField()->doubleClicked.notify(
				is2d_ ? mCB(this,uiSeisFileMan,man2DPush)
				      : mCB(this,uiSeisFileMan,browsePush) );

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();

    copybut_ = manipgrp->addButton( "copyobj", is2d ? "Copy dataset"
						    : "Copy cube",
				    mCB(this,uiSeisFileMan,copyPush) );
    if ( is2d )
	man2dlines_ = manipgrp->addButton( "man2d", "Manage lines",
					   mCB(this,uiSeisFileMan,man2DPush) );
    else
    {
	mergecubesbut_ = manipgrp->addButton( "mergeseis",
					"Merge cube parts into one cube",
					mCB(this,uiSeisFileMan,mergePush) );
	browsebut_ = manipgrp->addButton( "browseseis",
				"Browse/edit this cube",
				mCB(this,uiSeisFileMan,browsePush) );
    }

    attribbut_ = manipgrp->addButton( "attributes", "Show AttributeSet",
				      mCB(this,uiSeisFileMan,showAttribSet) );

    mTriggerInstanceCreatedNotifier();
    selChg(0);
}


uiSeisFileMan::~uiSeisFileMan()
{
}

#define mIsOfTranslType(typ) \
(FixedString(curioobj_->translator()) == typ##SeisTrcTranslator::translKey())


void uiSeisFileMan::ownSelChg()
{
    setToolButtonProperties();
}


#define mSetButToolTip(but,str1,curattribnms,str2,deftt) \
    if ( but ) \
    { \
	if ( but->sensitive() ) \
	{ \
	    tt.setEmpty(); \
	    tt.add( str1 ).add( curattribnms ).add( str2 ); \
	    but->setToolTip( tr(tt) ); \
	} \
	else \
	    but->setToolTip( deftt ); \
    }

void uiSeisFileMan::setToolButtonProperties()
{
    BufferString cursel;
    if ( curioobj_ )
	cursel.add( curioobj_->name() );

    BufferString tt;
    copybut_->setSensitive( !cursel.isEmpty() );
    mSetButToolTip(copybut_,"Make a Copy of '",cursel,"'",
		   is2d_ ? "Copy dataset" : "Copy cube");
    if ( browsebut_ )
    {
	const bool enabbrowse = curimplexists_ && mIsOfTranslType(CBVS);
	const bool issegydirect = curimplexists_ && mIsOfTranslType(SEGYDirect);
	browsebut_->setSensitive( enabbrowse || issegydirect );
	if ( enabbrowse )
	{
	mSetButToolTip(browsebut_,"Browse/edit '",cursel,"'",
			"Browse/edit selected cube");
	}
	else if ( issegydirect )
	{
	    mSetButToolTip(browsebut_,"Change location/name of SEGY files in '",
			   cursel,"'", "Change SEGY file for selected cube");
	}
    }

    if ( mergecubesbut_ )
    {
	BufferStringSet selcubenms;
	selgrp_->getChosen( selcubenms );
	if ( selcubenms.size() > 1 )
	{
	    mSetButToolTip(mergecubesbut_,"Merge ",
			   selcubenms.getDispString(2),"", "Merge cubes");
	}
	else
	    mergecubesbut_->setToolTip( "Merge cubes" );
    }

    if ( man2dlines_ )
    {
	man2dlines_->setSensitive( !cursel.isEmpty() );
	mSetButToolTip(man2dlines_,"Manage 2D lines in '",cursel,"'",
		       "Manage lines");
    }

    if ( attribbut_ )
    {
	attribbut_->setSensitive( curioobj_ );
	if ( curioobj_ )
	{
	     FilePath fp( curioobj_->fullUserExpr() );
	     fp.setExtension( "proc" );
	     attribbut_->setSensitive( File::exists(fp.fullPath()) );
	     mSetButToolTip(attribbut_,"Show AttributeSet for ",cursel,"",
			    "Show AttributeSet");
	}
	else
	    attribbut_->setToolTip( "Show AttributeSet" );
    }
}


void uiSeisFileMan::mkFileInfo()
{
    BufferString txt;
    SeisIOObjInfo oinf( curioobj_ );

    if ( oinf.isOK() )
    {

    if ( is2d_ )
    {
	BufferStringSet nms;
	SeisIOObjInfo::Opts2D opts2d; opts2d.zdomky_ = "*";
	oinf.getLineNames( nms, opts2d );
	txt += "Number of lines: "; txt += nms.size();
    }

#define mAddRangeTxt(line) \
    .add(" range: ").add(cs.hrg.start.line).add(" - ") \
    .add(cs.hrg.stop.line) \
    .add(" [").add(cs.hrg.step.line).add("]")
#define mAddZValTxt(memb) .add(zistm ? mNINT32(1000*memb) : memb)

    const bool zistm = oinf.isTime();
    const ZDomain::Def& zddef = oinf.zDomainDef();
    CubeSampling cs;
    if ( !is2d_ )
    {
	if ( oinf.getRanges(cs) )
	{
	    txt.setEmpty();
	    if ( !mIsUdf(cs.hrg.stop.inl()) )
		{ txt.add(sKey::Inline()) mAddRangeTxt(inl()); }
	    if ( !mIsUdf(cs.hrg.stop.crl()) )
		{ txt.addNewLine().add(sKey::Crossline()) mAddRangeTxt(crl()); }
	    float area = SI().getArea( cs.hrg.inlRange(), cs.hrg.crlRange() );
	    txt.add("\nArea: ").add( getAreaString( area, true, 0 ) );

	    txt.add("\n").add(zddef.userName()).add(" range ")
		.add(zddef.unitStr(true)).add(": ") mAddZValTxt(cs.zrg.start)
		.add(" - ") mAddZValTxt(cs.zrg.stop)
		.add(" [") mAddZValTxt(cs.zrg.step) .add("]");
	}
    }

    if ( !curioobj_->pars().isEmpty() )
    {
	const IOPar& pars = curioobj_->pars();
	FixedString parstr = pars.find( "Type" );
	if ( !parstr.isEmpty() )
	    txt.add( "\nType: " ).add( parstr );

	parstr = pars.find( "Optimized direction" );
	if ( !parstr.isEmpty() )
	    txt.add( "\nOptimized direction: " ).add( parstr );
	if ( pars.isTrue("Is Velocity") )
	{
	    Interval<float> topvavg, botvavg;
	    txt += "\nVelocity Type: ";
	    parstr = pars.find( "Velocity Type" );
	    txt += parstr.isEmpty() ? "<unknown>" : parstr.str();

	    if ( pars.get(VelocityStretcher::sKeyTopVavg(),topvavg)
	      && pars.get(VelocityStretcher::sKeyBotVavg(),botvavg))
	    {
		const StepInterval<float> sizrg = SI().zRange(true);
		StepInterval<float> dispzrg;

		if ( SI().zIsTime() )
		{
		    dispzrg.start = sizrg.start * topvavg.start / 2;
		    dispzrg.stop = sizrg.stop * botvavg.stop / 2;
		    dispzrg.step = (dispzrg.stop-dispzrg.start)
					/ sizrg.nrSteps();
		    txt.add( "\nDepth Range " )
			.add( ZDomain::Depth().unitStr(true) );
		}

		else
		{
		    dispzrg.start = 2 * sizrg.start / topvavg.stop;
		    dispzrg.stop = 2 * sizrg.stop / botvavg.start;
		    dispzrg.step = (dispzrg.stop-dispzrg.start)
					/ sizrg.nrSteps();
		    dispzrg.scale( (float)ZDomain::Time().userFactor() );
		    txt.add( "\nTime Range " )
			.add( ZDomain::Time().unitStr(true) );
		}

		txt.add( ": " ).add( dispzrg.start )
		    .add( " - " ).add( dispzrg.stop );
	    }
	}
    }

    if ( mIsOfTranslType(CBVS) )
    {
	CBVSSeisTrcTranslator* tri = CBVSSeisTrcTranslator::getInstance();
	if ( tri->initRead( new StreamConn(curioobj_->fullUserExpr(true),
				Conn::Read) ) )
	{
	    const BasicComponentInfo& bci =
		*tri->readMgr()->info().compinfo_[0];
	    const DataCharacteristics::UserType ut = bci.datachar.userType();
	    BufferString etxt = DataCharacteristics::getUserTypeString(ut);
	    txt.add( "\nStorage: " ).add( etxt.buf() + 4 );
	}
	delete tri;
    }

    const int nrcomp = oinf.nrComponents();
    if ( nrcomp > 1 )
	txt.add( "\nNumber of components: " ).add( nrcomp );


    } // if ( oinf.isOK() )

    if ( txt.isEmpty() )
	txt = "<No specific info available>\n";
    txt.add( "\n" ).add( getFileInfo() );

    setInfo( txt );
}


double uiSeisFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( !File::isDirectory(filenm) && File::isEmpty(filenm) ) return -1;

    double totalsz = 0;
    nrfiles = 0;
    if ( File::isDirectory(filenm) )
    {
	DirList dl( filenm, DirList::FilesOnly );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    FilePath filepath = dl.fullPath( idx );
	    FixedString ext = filepath.extension();
	    if ( ext != "cbvs" )
		continue;

	    totalsz += (double)File::getKbSize( filepath.fullPath() );
	    nrfiles++;
	}
    }
    else
    {
	while ( true )
	{
	    BufferString fullnm( CBVSIOMgr::getFileName(filenm,nrfiles) );
	    if ( !File::exists(fullnm) ) break;

	    totalsz += (double)File::getKbSize( fullnm );
	    nrfiles++;
	}
    }

    return totalsz;
}


void uiSeisFileMan::mergePush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    TypeSet<MultiID> chsnmids;
    selgrp_->getChosen( chsnmids );
    uiMergeSeis dlg( this );
    dlg.setInputIds( chsnmids );
    dlg.go();
    selgrp_->fullUpdate( key );
}


void uiSeisFileMan::dump2DPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    uiSeisDump2DGeom dlg( this, curioobj_ );
    dlg.go();
}


void uiSeisFileMan::browsePush( CallBacker* )
{
    if ( !curioobj_ ) return;
    const bool enabbrowse = curimplexists_ && mIsOfTranslType(CBVS);
    const bool issegydirect = curimplexists_ && mIsOfTranslType(SEGYDirect);
    if ( enabbrowse )
	uiSeisBrowser::doBrowse( this, *curioobj_, false );
    else if ( issegydirect )
    {
	uiEditSEGYFileDataDlg dlg( this, *curioobj_ );
	dlg.go();
    }
}


void uiSeisFileMan::man2DPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiSeis2DFileMan dlg( this, *curioobj_ );
    dlg.go();

    selgrp_->fullUpdate( key );
}


void uiSeisFileMan::copyPush( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const MultiID key( curioobj_->key() );
    bool needrefresh = false;
    if ( is2d_ )
    {
	uiSeisCopyLineSet dlg2d( this, curioobj_ );
	needrefresh = dlg2d.go();
    }
    else
    {
	uiSeisCopyCube dlg( this, curioobj_ );
	needrefresh = dlg.go();
    }

    if ( needrefresh )
	selgrp_->fullUpdate( key );
}


void uiSeisFileMan::manPS( CallBacker* )
{
    uiSeisPreStackMan dlg( this, is2d_ );
    dlg.go();
}


void uiSeisFileMan::showAttribSet( CallBacker* )
{
    if ( !curioobj_ ) return;

    FilePath fp( curioobj_->fullUserExpr() );
    fp.setExtension( "proc" );
    File::launchViewer( fp.fullPath(), File::ViewPars() );
}
