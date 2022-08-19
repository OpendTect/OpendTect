/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisfileman.h"

#include "cbvsreadmgr.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "seis2dlineio.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seistrctr.h"
#include "separstr.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "trckeyzsampling.h"
#include "zdomain.h"

#include "ui2dgeomman.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uimergeseis.h"
#include "uiseis2dfileman.h"
#include "uiseis2dfrom3d.h"
#include "uiseis2dgeom.h"
#include "uiseisbrowser.h"
#include "uiseiscopy.h"
#include "uiseisioobjinfo.h"
#include "uiseispsman.h"
#include "uisplitter.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

mDefineInstanceCreatedNotifierAccess(uiSeisFileMan)

class uiSeisCBVSBrowerMgr : public CallBacker
{ mODTextTranslationClass(uiSeisCBVSBrowerMgr)
public:

uiSeisCBVSBrowerMgr()
{
    uiSeisFileMan::BrowserDef* bdef = new uiSeisFileMan::BrowserDef(
			    CBVSSeisTrcTranslator::translKey() );
    bdef->tooltip_ = tr("Browse/Edit CBVS cube '%1'");
    bdef->cb_ = mCB(this,uiSeisCBVSBrowerMgr,doBrowse);
    uiSeisFileMan::addBrowser( bdef );
}

void doBrowse( CallBacker* cb )
{
    mDynamicCastGet(uiSeisFileMan*,sfm,cb)
    if ( sfm && sfm->curIOObj() )
	uiSeisBrowser::doBrowse( sfm, *sfm->curIOObj(), false );
}

};

static uiSeisCBVSBrowerMgr* cbvsbrowsermgr_ = 0;


#define mHelpID is2d ? mODHelpKey(mSeisFileMan2DHelpID) : \
		       mODHelpKey(mSeisFileMan3DHelpID)
uiSeisFileMan::uiSeisFileMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup(is2d
	    ? uiStrings::phrManage(tr("2D Seismics"))
	    : uiStrings::phrManage(tr("3D Seismics")),
				    mNoDlgTitle,mHelpID)
				    .nrstatusflds(1).modal(false),
		  SeisTrcTranslatorGroup::ioContext(),
		is2d ? "2D Seismic Data" : "Seismic Data" )
    , is2d_(is2d)
{
    if ( !cbvsbrowsermgr_ )
	cbvsbrowsermgr_ = new uiSeisCBVSBrowerMgr;

    IOObjContext* freshctxt = Seis::getIOObjContext(
					is2d_ ? Seis::Line : Seis::Vol, true );
    ctxt_ = *freshctxt;
    delete freshctxt;

    createDefaultUI( true );
    selgrp_->getListField()->doubleClicked.notify(
				is2d_ ? mCB(this,uiSeisFileMan,man2DPush)
				      : mCB(this,uiSeisFileMan,browsePush) );

    copybut_ = addManipButton( "copyobj", is2d ? uiStrings::phrCopy(
				tr("dataset")) : uiStrings::phrCopy(tr("cube")),
				mCB(this,uiSeisFileMan,copyPush) );
    if ( is2d )
    {
	man2dlinesbut_ = addManipButton( "man2d",
				uiStrings::phrManage(uiStrings::sLine(2)),
				mCB(this,uiSeisFileMan,man2DPush) );
	if ( SI().has3D() )
	    addManipButton( "extr3dinto2d", tr("Extract from 3D cube"),
			mCB(this,uiSeisFileMan,extrFrom3D) );
    }
    else
    {
	mergecubesbut_ = addManipButton( "mergeseis",uiStrings::phrMerge(
					tr("cube parts into one cube")),
					mCB(this,uiSeisFileMan,mergePush) );
	browsebut_ = addManipButton( "browseseis",
				tr("Browse/edit this cube"),
				mCB(this,uiSeisFileMan,browsePush) );
    }

    attribbut_ = addManipButton( "attributes", sShowAttributeSet(),
				 mCB(this,uiSeisFileMan,showAttribSet) );
    segyhdrbut_ = addManipButton( "segy", tr("Show SEG-Y EBCDIC Header"),
				  mCB(this,uiSeisFileMan,showSEGYHeader) );

    mTriggerInstanceCreatedNotifier();
    mAttachCB( postFinalize(), uiSeisFileMan::selChg );
}


uiSeisFileMan::~uiSeisFileMan()
{
    detachAllNotifiers();
}


static ObjectSet<uiSeisFileMan::BrowserDef> browserdefs_;


int uiSeisFileMan::addBrowser( uiSeisFileMan::BrowserDef* bd )
{
    browserdefs_ += bd;
    return browserdefs_.size() - 1;
}


#define mIsOfTranslName(nm) (StringView(curioobj_->translator()) == nm)
#define mIsOfTranslType(typ) \
	mIsOfTranslName(typ##SeisTrcTranslator::translKey())


void uiSeisFileMan::ownSelChg()
{
    setToolButtonProperties();
}


#define mSetButToolTip(but,str1,curattribnms,str2,deftt) { \
    if ( but ) \
    { \
	if ( but->sensitive() ) \
	{ \
	    tt.setEmpty(); \
	    tt.append( str1 ).append( curattribnms ).append( str2 ); \
	    but->setToolTip( tt ); \
	} \
	else \
	    but->setToolTip( deftt ); \
    } }

void uiSeisFileMan::setToolButtonProperties()
{
    BufferString cursel;
    if ( curioobj_ )
	cursel.add( curioobj_->name() );

    uiString tt;
    copybut_->setSensitive( !cursel.isEmpty() );
    mSetButToolTip(copybut_,tr("Make a Copy of '"),toUiString(cursel),
		   toUiString("'"), is2d_ ? uiStrings::phrCopy(tr("dataset")) :
		   uiStrings::phrCopy(uiStrings::sCube().toLower()));
    if ( browsebut_ )
    {
	const BrowserDef* bdef = getBrowserDef();
	const bool enabbrowse = curimplexists_ && bdef;
	browsebut_->setSensitive( enabbrowse );
	if ( !enabbrowse )
	    mSetButToolTip( browsebut_, tr("No browser for '"),
			    toUiString(cursel), toUiString("'"),
			    tr("Browse/edit selected cube") )
	else
	{
	    uiString bdeftt( bdef->tooltip_ );
	    browsebut_->setToolTip( bdeftt.arg(curioobj_->uiName()) );
	}
    }

    if ( mergecubesbut_ )
    {
	BufferStringSet selcubenms;
	selgrp_->getChosen( selcubenms );
	if ( selcubenms.size() > 1 )
	    mSetButToolTip(mergecubesbut_,uiStrings::sMerge(),toUiString(" %1")
			   .arg(toUiString(selcubenms.getDispString(2))),
			   uiStrings::sEmptyString(),uiStrings::phrMerge(
			   uiStrings::sCube().toLower()))
	else
	    mergecubesbut_->setToolTip( uiStrings::phrMerge(
					    uiStrings::sCube(2).toLower()) );
    }

    if ( man2dlinesbut_ )
    {
	man2dlinesbut_->setSensitive( !cursel.isEmpty() );
	mSetButToolTip(man2dlinesbut_,uiStrings::phrManage(tr("2D lines in '")),
		       toUiString(cursel),toUiString("'"),
		       uiStrings::phrManage(uiStrings::sLine(2)))
    }

    attribbut_->setSensitive( curioobj_ );
    if ( curioobj_ )
    {
	 FilePath fp( curioobj_->fullUserExpr() );
	 fp.setExtension( "proc" );
	 attribbut_->setSensitive( File::exists(fp.fullPath()) );
	 mSetButToolTip(attribbut_,tr("Show AttributeSet for "),
			toUiString(cursel),uiStrings::sEmptyString(),
			sShowAttributeSet())
    }
    else
	attribbut_->setToolTip( sShowAttributeSet() );

    segyhdrbut_->setSensitive( curioobj_ );
    if ( curioobj_ )
    {
	 FilePath fp( curioobj_->fullUserExpr() );
	 fp.setExtension( "sgyhdr" );
	 segyhdrbut_->setSensitive( File::exists(fp.fullPath()) );
    }
}


const uiSeisFileMan::BrowserDef* uiSeisFileMan::getBrowserDef() const
{
    if ( curioobj_ )
    {
	for ( int idx=0; idx<browserdefs_.size(); idx++ )
	{
	    const BrowserDef* bdef = browserdefs_[idx];
	    if ( bdef->name_ == curioobj_->translator() )
		return bdef;
	}
    }

    return nullptr;
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
    .add(" range: ").add(cs.hsamp_.start_.line).add(" - ") \
    .add(cs.hsamp_.stop_.line) \
    .add(" [").add(cs.hsamp_.step_.line).add("]")

    const ZDomain::Def& zddef = oinf.zDomainDef();
    TrcKeyZSampling cs;
    if ( !is2d_ )
    {
	if ( oinf.getRanges(cs) )
	{
	    txt.setEmpty();
	    if ( !mIsUdf(cs.hsamp_.stop_.inl()) )
		{ txt.add(sKey::Inline()) mAddRangeTxt(inl()); }
	    if ( !mIsUdf(cs.hsamp_.stop_.crl()) )
		{ txt.addNewLine().add(sKey::Crossline()) mAddRangeTxt(crl()); }
	    SeisIOObjInfo::SpaceInfo spcinfo;
	    double area;
	    if ( oinf.getDefSpaceInfo(spcinfo) )
	    {
		area = sCast(double,cs.hsamp_.lineDistance()) *
			 cs.hsamp_.trcDistance() * spcinfo.expectednrtrcs;
	    }
	    else
	    {
		area = sCast(double,SI().getArea(cs.hsamp_.inlRange(),
						 cs.hsamp_.crlRange()));
	    }
	    txt.add("\nArea: ")
	       .add( getAreaString(sCast(float,area),SI().xyInFeet(),2,true) );

	    StepInterval<float> zrg = cs.zsamp_;
	    zrg.scale( zddef.userFactor() );
	    const int nrdec = Math::NrSignificantDecimals( zrg.step );
	    txt.add("\n").add(mFromUiStringTodo(zddef.getRange()))
		.add(zddef.unitStr(true)).add(": ").add( zrg.start, nrdec )
		.add(" - ").add( zrg.stop, nrdec )
		.add(" [").add( zrg.step, nrdec ).add("]");
	}
    }

    if ( !curioobj_->pars().isEmpty() )
    {
	const IOPar& pars = curioobj_->pars();
	StringView parstr = pars.find( "Type" );
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

    BufferString dsstr = curioobj_->pars().find( sKey::DataStorage() );
    if ( mIsOfTranslType(CBVS) )
    {
	CBVSSeisTrcTranslator* tri = CBVSSeisTrcTranslator::getInstance();
	if ( tri->initRead( new StreamConn(curioobj_->fullUserExpr(true),
				Conn::Read) ) )
	{
	    const BasicComponentInfo& bci =
		*tri->readMgr()->info().compinfo_[0];
	    const DataCharacteristics::UserType ut = bci.datachar.userType();
	    dsstr = DataCharacteristics::getUserTypeString(ut);
	}
	delete tri;
    }
    if ( dsstr.size() > 4 )
	txt.add( "\nStorage: " ).add( dsstr.buf() + 4 );

    const int nrcomp = oinf.nrComponents();
    if ( nrcomp > 1 )
	txt.add( "\nNumber of components: " ).add( nrcomp );


    } // if ( oinf.isOK() )

    if ( txt.isEmpty() )
	txt = "<No specific info available>\n";
    txt.add( "\n" ).add( getFileInfo() );

    setInfo( txt );
}


od_int64 uiSeisFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( !File::isDirectory(filenm) && File::isEmpty(filenm) ) return -1;

    od_int64 totalsz = 0;
    nrfiles = 0;
    if ( File::isDirectory(filenm) )
    {
	DirList dl( filenm, File::FilesInDir );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    FilePath filepath = dl.fullPath( idx );
	    StringView ext = filepath.extension();
	    if ( ext != "cbvs" )
		continue;

	    totalsz += File::getKbSize( filepath.fullPath() );
	    nrfiles++;
	}
    }
    else
    {
	while ( true )
	{
	    BufferString fullnm( CBVSIOMgr::getFileName(filenm,nrfiles) );
	    if ( !File::exists(fullnm) ) break;

	    totalsz += File::getKbSize( fullnm );
	    nrfiles++;
	}
    }

    return totalsz;
}


void uiSeisFileMan::mergePush( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const MultiID key( curioobj_->key() );
    TypeSet<MultiID> chsnmids;
    selgrp_->getChosen( chsnmids );
    uiMergeSeis dlg( this );
    dlg.setInputIds( chsnmids );
    if ( dlg.go() )
	selgrp_->fullUpdate( key );
}


void uiSeisFileMan::browsePush( CallBacker* )
{
    const BrowserDef* bdef = getBrowserDef();
    if ( bdef )
    {
	CallBack cb( bdef->cb_ );
	cb.doCall( this );
    }
}


void uiSeisFileMan::man2DPush( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const MultiID key( curioobj_->key() );
    uiSeis2DFileMan dlg( this, *curioobj_ );
    if ( dlg.go() )
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
	uiSeisCopy2DDataSet dlg2d( this, curioobj_ );
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
    if ( !curioobj_ )
	return;

    FilePath fp( curioobj_->fullUserExpr() );
    fp.setExtension( "proc" );
    File::launchViewer( fp.fullPath(), File::ViewPars() );
}


void uiSeisFileMan::showSEGYHeader( CallBacker* )
{
    if ( !curioobj_ )
	return;

    FilePath fp( curioobj_->fullUserExpr() );
    fp.setExtension( "sgyhdr" );
    File::launchViewer( fp.fullPath(), File::ViewPars() );
}


void uiSeisFileMan::extrFrom3D( CallBacker* )
{
    uiSeis2DFrom3D dlg( this );
    if ( dlg.go() )
	updateCB( nullptr );
}
