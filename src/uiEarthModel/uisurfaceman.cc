/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisurfaceman.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "multiid.h"
#include "survinfo.h"
#include "unitofmeasure.h"

#include "embody.h"
#include "embodytr.h"
#include "emfault3d.h"
#include "emfaultauxdata.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceio.h"
#include "emsurfacetr.h"

#include "uibodyoperatordlg.h"
#include "uibodyregiondlg.h"
#include "uicolor.h"
#include "uifaultsetcopy.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uihorizonmergedlg.h"
#include "uihorizonrelations.h"
#include "uiimpbodycaldlg.h"
#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uiioobjseldlg.h"
#include "uiioobjselgrp.h"
#include "uiiosurfacedlg.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "uitable.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "od_helpids.h"




mDefineInstanceCreatedNotifierAccess(uiSurfaceMan)



#define mCaseRetCtxt(enm,trgrpnm) \
    case EM::ObjectType::enm: return trgrpnm##TranslatorGroup::ioContext()

static IOObjContext getIOCtxt( EM::ObjectType typ )
{
    switch ( typ )
    {
	mCaseRetCtxt(Hor2D,EMHorizon2D);
	mCaseRetCtxt(Hor3D,EMHorizon3D);
	mCaseRetCtxt(AnyHor,EMAnyHorizon);
	mCaseRetCtxt(FltSS2D,EMFaultStickSet);
	mCaseRetCtxt(FltSS3D,EMFaultStickSet);
	mCaseRetCtxt(FltSS2D3D,EMFaultStickSet);
	mCaseRetCtxt(Flt3D,EMFault3D);
	mCaseRetCtxt(FltSet,EMFaultSet3D);
	default:
	mCaseRetCtxt(Body,EMBody);
    }
}

#define mCaseRetStr(enm,str) \
    case EM::ObjectType::enm: return toUiString("%1 %2").arg(act).arg(str);

static uiString getActStr( EM::ObjectType typ, const uiString& act )
{
    switch ( typ )
    {
	mCaseRetStr( Hor2D, EMHorizon2DTranslatorGroup::sTypeName() );
	mCaseRetStr( Hor3D, EMHorizon3DTranslatorGroup::sTypeName() );
	mCaseRetStr( FltSS2D, uiStrings::sFaultStickSet2D() );
	mCaseRetStr( FltSS3D, uiStrings::sFaultStickSet3D() );
	mCaseRetStr( FltSS2D3D, uiStrings::sFaultStickSet() );
	mCaseRetStr( Flt3D, uiStrings::sFault() );
	mCaseRetStr( FltSet, uiStrings::sFaultSet() );
	mCaseRetStr( Body, uiStrings::sGeobody() );
	default:
	mCaseRetStr( AnyHor, uiStrings::sHorizon(1) );
    }
}


static HelpKey getHelpID( EM::ObjectType typ )
{
    switch ( typ )
    {
	case EM::ObjectType::Hor2D:
			return mODHelpKey(mSurface2DManHelpID);
	case EM::ObjectType::FltSS2D:
	case EM::ObjectType::FltSS3D:
	case EM::ObjectType::FltSS2D3D:
			return mODHelpKey(mFaultStickSetsManageHelpID);
	case EM::ObjectType::Flt3D:
			return mODHelpKey(mFaultsManageHelpID);
	case EM::ObjectType::Body:
			return mODHelpKey(mBodyManHelpID);
	default:	return mODHelpKey(mSurfaceManHelpID);
    }
}


uiSurfaceMan::uiSurfaceMan( uiParent* p, EM::ObjectType typ )
    : uiObjFileMan(p,Setup(getActStr(typ,tr("Manage")),
			   getHelpID(typ)).nrstatusflds(1).modal(false),
		   getIOCtxt(typ),ZDomain::sKey())
    , type_(typ)
    , attribfld_(nullptr)
    , man2dbut_(0)
    , surfdatarenamebut_(0)
    , surfdataremovebut_(0)
    , copybut_(0)
    , mergehorbut_(0)
    , applybodybut_(0)
    , createregbodybut_(0)
    , volestimatebut_(0)
    , switchvalbut_(0)
{
    createDefaultUI();
    if ( type_ != EM::ObjectType::Body )
	copybut_ = addManipButton( "copyobj", tr("Copy to new object"),
					mCB(this,uiSurfaceMan,copyCB) );

    if ( type_ == EM::ObjectType::Hor2D ||
					type_ == EM::ObjectType::AnyHor )
    {
	man2dbut_ = addManipButton( "man2d",
	  uiStrings::phrManage( EMHorizon2DTranslatorGroup::sTypeName(mPlural)),
				mCB(this,uiSurfaceMan,man2dCB) );
	man2dbut_->setSensitive( false );
    }

    if ( type_ == EM::ObjectType::Hor3D )
    {
	mergehorbut_ = addManipButton( "mergehorizons",
		uiStrings::phrMerge(uiStrings::phrJoinStrings(
				    uiStrings::s3D(), uiStrings::sHorizon(2))),
		mCB(this,uiSurfaceMan,merge3dCB) );
    }

    if ( type_ == EM::ObjectType::Hor3D ||
					    type_ == EM::ObjectType::AnyHor )
    {
	uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Horizon Data"),
			     uiListBox::AboveMid );
	attribfld_ = new uiListBox( listgrp_, su, "horizondata" );
	attribfld_->attach( rightOf, selgrp_ );
	attribfld_->setHSzPol( uiObject::Wide );
	attribfld_->box()->setToolTip(
		tr("Horizon Data (Attributes stored in Horizon format)") );
	attribfld_->selectionChanged.notify( mCB(this,uiSurfaceMan,attribSel) );

	uiManipButGrp* butgrp = new uiManipButGrp( attribfld_ );
	surfdataremovebut_ = butgrp->addButton( uiManipButGrp::Remove,
					tr("Remove selected Horizon Data"),
					mCB(this,uiSurfaceMan,removeAttribCB) );
	surfdatarenamebut_ = butgrp->addButton( uiManipButGrp::Rename,
					tr("Rename selected Horizon Data"),
					mCB(this,uiSurfaceMan,renameAttribCB) );
	butgrp->attach( rightTo, attribfld_->box() );

	new uiPushButton( extrabutgrp_, uiStrings::sStratigraphy(),
		mCB(this,uiSurfaceMan,stratSel), false );
	extrabutgrp_->attach( ensureBelow, attribfld_ );
	setPrefWidth( 50 );
    }

    if ( type_==EM::ObjectType::Hor3D || type_==EM::ObjectType::Hor2D )
    {
	new uiPushButton( extrabutgrp_, tr("Relations"),
		mCB(this,uiSurfaceMan,setRelations), false );
    }

    if ( type_ == EM::ObjectType::Body )
    {
	applybodybut_ = addManipButton( "set_union",
					tr("Apply Geobody operations"),
					mCB(this,uiSurfaceMan,mergeBodyCB) );
	createregbodybut_ = addManipButton( "set_implicit",
				tr("Create geobody/region"),
				mCB(this,uiSurfaceMan,createBodyRegionCB) );
	volestimatebut_ = addManipButton( "bodyvolume",
					  tr("Volume estimate"),
					  mCB(this,uiSurfaceMan,calcVolCB) );
	switchvalbut_ = addManipButton( "switch_implicit",
					tr("Switch inside/outside value"),
					mCB(this,uiSurfaceMan,switchValCB) );
    }

    if ( type_ == EM::ObjectType::FltSet )
    {
	manselsetbut_ = addManipButton( "man_flt",
			    uiStrings::phrManage(uiStrings::sFault(mPlural)),
			    mCB(this,uiSurfaceMan,manFltSetCB) );
	manselsetbut_->setSensitive( false );
    }

    if ( type_==EM::ObjectType::Flt3D || type_==EM::ObjectType::FltSet )
    {
	addManipButton( "faultplanes", tr("Copy Faults to FaultSet"),
			mCB(this,uiSurfaceMan,copyFault2FaultSetCB) );
    }

    mTriggerInstanceCreatedNotifier();
}


uiSurfaceMan::~uiSurfaceMan()
{
}


void uiSurfaceMan::ownSelChg()
{
    setToolButtonProperties();

    if ( selgrp_->isEmpty() && attribfld_ )
	attribfld_->setEmpty();
}


void uiSurfaceMan::attribSel( CallBacker* )
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
	{ \
	    but->setToolTip( deftt ); \
	} \
    }
uiString uiSurfaceMan::sRenameSelData()
{
    return tr("Rename selected data");
}


uiString uiSurfaceMan::sRemoveSelData()
{
    return tr("Remove selected data");
}


void uiSurfaceMan::setToolButtonProperties()
{
    const bool hasattribs = attribfld_ && !attribfld_->isEmpty();

    BufferString tt, cursel;

    if ( curioobj_ )
	cursel.add( curioobj_->name() );

    if ( surfdatarenamebut_ )
    {
	surfdatarenamebut_->setSensitive( hasattribs );
	BufferString nm;
	if ( attribfld_ )
	    nm = attribfld_->getText();
	mSetButToolTip(surfdatarenamebut_,"Rename '",nm,"'",sRenameSelData())
    }

    if ( surfdataremovebut_ )
    {
	surfdataremovebut_->setSensitive( hasattribs );
	BufferStringSet attrnms;
	if ( attribfld_ )
	    attribfld_->getChosen( attrnms );
	mSetButToolTip(surfdataremovebut_,"Remove ",attrnms.getDispString(2),
		       "", sRemoveSelData())
    }

    if ( copybut_ )
    {
	copybut_->setSensitive( curioobj_ );
	mSetButToolTip(copybut_,"Copy '",cursel,"' to new object",
		       uiStrings::phrCopy(tr("to new object")))
    }

    if ( mergehorbut_ )
    {
	mergehorbut_->setSensitive( curioobj_ );
	BufferStringSet selhornms;
	selgrp_->getChosen( selhornms );
	if ( selhornms.size() > 1 )
	{
	    mSetButToolTip(mergehorbut_,"Merge ",selhornms.getDispString(2),
			   "", uiStrings::phrMerge(uiStrings::phrJoinStrings(
			   uiStrings::s3D(),uiStrings::sHorizon(2))))
	}
	else
	    mergehorbut_->setToolTip(  uiStrings::phrMerge(
				    uiStrings::phrJoinStrings(uiStrings::s3D(),
				    uiStrings::sHorizon(2))) );
    }

     if ( type_ == EM::ObjectType::Body )
     {
	 applybodybut_->setSensitive( curioobj_ );
	 volestimatebut_->setSensitive( curioobj_ );
	 switchvalbut_->setSensitive( curioobj_ );
	 mSetButToolTip(volestimatebut_,"Estimate volume of '",cursel,
			"'", tr("Volume estimate"));
	 mSetButToolTip(switchvalbut_,"Switch inside/outside value of '",
			cursel,"'", tr("Switch inside/outside value"));
     }
}


bool uiSurfaceMan::isCur2D() const
{
    return curioobj_ &&
	   curioobj_->group() == EMHorizon2DTranslatorGroup::sGroupName();
}


bool uiSurfaceMan::isCurFault() const
{
    const BufferString grp = curioobj_ ? curioobj_->group().buf() : "";
    return grp==EMFaultStickSetTranslatorGroup::sGroupName() ||
	   grp==EMFault3DTranslatorGroup::sGroupName();
}


void uiSurfaceMan::copyCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    PtrMan<IOObj> ioobj = curioobj_->clone();
    if ( type_ == EM::ObjectType::FltSet )
    {
	uiCopyFaultSet dlg( this, *ioobj );
	if ( dlg.go() )
	    selgrp_->fullUpdate( ioobj->key() );

	return;
    }

    const bool canhaveattribs = type_ == EM::ObjectType::Hor3D;
    uiSurfaceRead::Setup su( ioobj->group() );
    su.withattribfld(canhaveattribs).withsubsel(!isCurFault())
      .multisubsel(true).withsectionfld(false);

    uiCopySurface dlg( this, *ioobj, su );
    if ( dlg.go() )
	selgrp_->fullUpdate( ioobj->key() );
}


void uiSurfaceMan::merge3dCB( CallBacker* )
{
    uiHorizonMergeDlg dlg( this, false );
    TypeSet<MultiID> chsnmids;
    selgrp_->getChosen( chsnmids );
    dlg.setInputHors( chsnmids );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getNewHorMid() );
}


void uiSurfaceMan::mergeBodyCB( CallBacker* )
{
    uiBodyOperatorDlg dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::calcVolCB( CallBacker* )
{
    if ( !curioobj_ )
	return;

    RefMan<EM::EMObject> emo =
	EM::EMM().loadIfNotFullyLoaded( curioobj_->key(), 0 );
    mDynamicCastGet( EM::Body*, emb, emo.ptr() );
    if ( !emb )
    {
	uiString msg = tr( "Geobody '%1' is empty" ).arg( curioobj_->uiName() );
	uiMSG().error(msg);
	return;
    }

    uiImplBodyCalDlg dlg( this, *emb );
    uiString dlgtitle = tr( "Geobody volume estimation for '%1'" )
			  .arg(curioobj_->uiName() );
    dlg.setTitleText( dlgtitle );
    dlg.go();
}


void uiSurfaceMan::createBodyRegionCB( CallBacker* )
{
    uiBodyRegionDlg dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::switchValCB( CallBacker* )
{
    uiImplicitBodyValueSwitchDlg dlg( this, curioobj_ );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::setRelations( CallBacker* )
{
    uiHorizonRelationsDlg dlg( this, isCur2D() );
    dlg.go();
}


void uiSurfaceMan::removeAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const bool isflt =
	curioobj_->group()==EMFault3DTranslatorGroup::sGroupName();
    uiString datatype =
	isflt ? uiStrings::sFaultData() : uiStrings::sHorizonData();
    if ( curioobj_->implReadOnly() )
    {
	uiMSG().error(
		tr("Cannot delete %1. Surface is read-only").arg(datatype) );
	return;
    }

    BufferStringSet attrnms;
    attribfld_->getChosen( attrnms );
    uiString msg = tr("%1 '%2'\nwill be deleted from disk. "
		      "Do you wish to continue?").arg(datatype)
			.arg(attrnms.getDispString(2));
    if ( !uiMSG().askGoOn(msg,uiStrings::sDelete(),uiStrings::sCancel()) )
	return;

    if ( isflt )
    {
	EM::FaultAuxData fad( curioobj_->key() );
	for ( int ida=0; ida<attrnms.size(); ida++ )
	    fad.removeData( attrnms.get(ida).buf() );
    }
    else
    {
	for ( int ida=0; ida<attrnms.size(); ida++ )
	    EM::SurfaceAuxData::removeFile( *curioobj_, attrnms.get(ida) );
    }

    selChg( this );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiSurfaceMan::renameAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const BufferString attribnm = attribfld_->getText();
    const uiString titl = tr("Rename '%1'").arg(attribnm);
    uiGenInputDlg dlg( this, titl, tr("New name"), new StringInpSpec(attribnm));
    if ( !dlg.go() ) return;

    const char* newnm = dlg.text();
    if ( attribfld_->isPresent(newnm) )
	mErrRet( tr("Name is already in use") )

    if ( curioobj_->group()==EMFault3DTranslatorGroup::sGroupName() )
    {
	EM::FaultAuxData fad( curioobj_->key() );
	fad.setDataName( attribnm.buf(), newnm );

	selChg( this );
	return;
    }

    const BufferString filename =
		EM::SurfaceAuxData::getFileName( *curioobj_, attribnm );
    if ( File::isEmpty(filename) )
	mErrRet( tr("Cannot find Horizon Data file") )
    else if ( !File::isWritable(filename) )
	mErrRet( tr("The Horizon Data file is not writable") )

    od_istream instrm( filename );
    if ( !instrm.isOK() )
	mErrRet( tr("Cannot open Horizon Data file for read") )
    const BufferString ofilename( filename, "_new" );
    od_ostream outstrm( ofilename );
    if ( !outstrm.isOK() )
	mErrRet( tr("Cannot open new Horizon Data file for write") )

    ascistream aistrm( instrm );
    ascostream aostrm( outstrm );
    aostrm.putHeader( aistrm.fileType() );
    IOPar iop( aistrm );
    iop.set( sKey::Attribute(), newnm );
    iop.putTo( aostrm );

    outstrm.add( instrm );
    const bool writeok = outstrm.isOK();
    instrm.close(); outstrm.close();

    BufferString tmpfnm( filename ); tmpfnm += "_old";
    if ( !writeok )
    {
	File::remove( ofilename );
	mErrRet( tr("Error during write. Reverting to old name") )
    }

    if ( File::rename(filename,tmpfnm) )
	File::rename(ofilename,filename);
    else
    {
	File::remove( ofilename );
	mErrRet( tr("Cannot rename file(s). Reverting to old name") )
    }

    if ( File::exists(tmpfnm) )
	File::remove( tmpfnm );

    selChg( this );
}


void uiSurfaceMan::fillAttribList()
{
    if ( !attribfld_ ) return;

    attribfld_->setEmpty();
    TypeSet<MultiID> mids;
    selgrp_->getChosen( mids );
    if ( mids.isEmpty() )
	return;

    const MultiID& firstmid = mids[0];
    EM::IOObjInfo info( firstmid );
    if ( !info.isOK() )
	return;

    BufferStringSet availableattrnms;
    if ( !info.getAttribNames( availableattrnms ) )
	return;

    for ( int midx=1; midx<mids.size(); midx++ )
    {
	const MultiID& mid = mids[midx];
	EM::IOObjInfo eminfo( mid );
	if ( !eminfo.isOK() )
	    return;

	BufferStringSet attrnms;
	eminfo.getAttribNames( attrnms );
	for ( int idx=availableattrnms.size()-1; idx>=0; idx-- )
	{
	    if ( !attrnms.isPresent(availableattrnms.get(idx)) )
		availableattrnms.removeSingle( idx );
	}
    }

    attribfld_->addItems( availableattrnms );
    attribfld_->chooseAll( false );
}


static void addZRangeTxt( const ZDomain::Info& zinfo, ZSampling zrange,
			  BufferString& txt )
{
    if ( zrange.isUdf() )
	return;

    BufferString zrngtext;
    const int nrdec = zrange.step_;
    const float zuserfac = zinfo.userFactor();
    zrange.scale( zuserfac );
    zrngtext.add( zinfo.getLabel() ).add(": ")
	    .add( toUiString("%1 - %2")
		    .arg( toUiString(zrange.start_,0,'f',nrdec) )
		    .arg( toUiString(zrange.stop_,0,'f',nrdec)  ) );

    txt.add( zrngtext ).addNewLine();
}


static void addInlCrlRangeTxt( BufferString& txt,
			       const StepInterval<int>& range )
{
    if ( range.isUdf() )
	txt.add( "-" ).addNewLine();
    else
        txt.add( range.start_ ).add( " - " ).add( range.stop_ )
                .add( " [" ).add( range.step_ ).add( "]" ).addNewLine();
}


void uiSurfaceMan::mkFileInfo()
{
    fillAttribList();
    BufferString txt;
    EM::IOObjInfo eminfo( curioobj_ );
    if ( !eminfo.isOK() )
    {
	txt += getFileInfo();
	setInfo( txt );
	return;
    }

    if ( man2dbut_ )
	man2dbut_->setSensitive( isCur2D() );

    if ( isCur2D() || isCurFault() )
    {
	txt = isCur2D() ? "Nr. 2D lines: " : "Nr. Sticks: ";
	if ( isCurFault() )
	{
	    if ( eminfo.nrSticks() < 0 )
		txt += "Cannot determine number of sticks for this object type";
	    else
		txt += eminfo.nrSticks();
	}
	else
	{
	    BufferStringSet linenames;
	    if ( eminfo.getLineNames(linenames) )
		txt += linenames.size();
	    else
		txt += "-";
	}

	txt += "\n";
    }
    else if ( type_ == EM::ObjectType::FltSet )
    {
	const DirList dl( curioobj_->fullUserExpr(),
			  File::DirListType::FilesInDir, "*.flt" );
	txt = "Nr Faults: ";
	txt += dl.size();
	txt += "\n";
	if ( manselsetbut_ )
        manselsetbut_->setSensitive( dl.size() );
    }
    else if ( type_ == EM::ObjectType::Body )
    {
	TrcKeyZSampling cs(false);
	if ( eminfo.getBodyRange(cs) )
	{
	    StepInterval<int> range = cs.hsamp_.lineRange();
	    txt = "In-line range: ";
	    addInlCrlRangeTxt( txt, range );
	    range = cs.hsamp_.trcRange();
	    txt += "Cross-line range: ";
	    addInlCrlRangeTxt( txt, range );
	    addZRangeTxt( eminfo.zDomain(), cs.zsamp_, txt );
	}
    }
    else
    {
	StepInterval<int> range = eminfo.getInlRange();
	txt = "In-line range: ";
	addInlCrlRangeTxt( txt, range );
	range = eminfo.getCrlRange();
	txt += "Cross-line range: ";
	addInlCrlRangeTxt( txt, range );
	const ZSampling zrg( eminfo.getZRange(),
			     eminfo.zDomain()
				.getReasonableZSampling(false,false).step_ );
	addZRangeTxt( eminfo.zDomain(), eminfo.getZRange(), txt);
    }

    txt += getFileInfo();

    setInfo( txt );
    setToolButtonProperties();
}


BufferString uiSurfaceMan::createFileName( const char* basenm, int idx )
{
    BufferString ret( basenm );
    ret.add( "^" ).add( idx ).add( ".hov" );
    return ret;
}


od_int64 uiSurfaceMan::getFileSize( const char* filenm ) const
{
    if ( type_ == EM::ObjectType::FltSet )
	return uiObjFileMan::getFileSize( filenm );

    if ( File::isEmpty(filenm) )
	return -1;

    od_int64 totalsz = File::getFileSize( filenm );
    const int nrfiles = getNrFiles( filenm );
    const BufferString basefnm( filenm );
    for ( int idx=0; idx<nrfiles-1; idx++ )
    {
	BufferString fnm = createFileName( basefnm, idx );
	if ( !File::exists(fnm) )
	    return -1;

	totalsz += File::getFileSize( fnm );
    }

    return totalsz;
}


int uiSurfaceMan::getNrFiles( const char* filenm ) const
{
    if ( type_ == EM::ObjectType::FltSet )
	return uiObjFileMan::getNrFiles( filenm );

    if ( File::isEmpty(filenm) )
	return 0;

    int nrfiles = 1;
    const BufferString basefnm( filenm );
    for ( int idx=0; ; idx++ )
    {
	BufferString fnm = createFileName( basefnm, idx );
	if ( !File::exists(fnm) )
	    break;

	nrfiles++;
    }

    return nrfiles;
}


class uiSurfaceStratDlg : public uiDialog
{ mODTextTranslationClass(uiSurfaceStratDlg);
public:
uiSurfaceStratDlg( uiParent* p, const TypeSet<MultiID>& ids )
    : uiDialog(p,Setup(uiStrings::sStratigraphy(),mNoHelpKey))
    , objids_(ids)
{
    tbl_ = new uiTable( this, uiTable::Setup(ids.size(),3),
			"Stratigraphy Table" );
    uiStringSet lbls; lbls.add( uiStrings::sName() ).add( uiStrings::sColor() )
                          .add( uiStrings::sLevel() );
    tbl_->setColumnLabels( lbls );
    tbl_->setTableReadOnly( true );
    tbl_->setRowResizeMode( uiTable::Interactive );
    tbl_->setColumnResizeMode( uiTable::ResizeToContents );
    tbl_->setColumnStretchable( 2, true );
    tbl_->setPrefWidth( 400 );
    tbl_->doubleClicked.notify( mCB(this,uiSurfaceStratDlg,doCol) );

    auto* sb = new uiToolButton( this, "man_strat",
				      tr("Edit Stratigraphy to define Markers"),
				      mCB(this,uiSurfaceStratDlg,doStrat) );
    sb->attach( rightOf, tbl_ );

    IOPar par;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	par.setEmpty();
	if ( !EM::EMM().readDisplayPars(ids[idx],par) )
	    continue;
	tbl_->setText( RowCol(idx,0), EM::EMM().objectName(ids[idx]) );

	OD::Color col( OD::Color::White() );
	par.get( sKey::Color(), col );
	tbl_->setColor( RowCol(idx,1), col );

	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, true,
						    uiStrings::sEmptyString() );
	levelsel->selChange.notify( mCB(this,uiSurfaceStratDlg,lvlChg) );
	tbl_->setCellGroup( RowCol(idx,2), levelsel );

	Strat::LevelID lvlid;
	par.get( sKey::StratRef(), lvlid );
	levelsel->setID( lvlid );
    }
}


protected:

void doStrat( CallBacker* )
{ StratTWin().popUp(); }

void doCol( CallBacker* )
{
    const RowCol& cell = tbl_->notifiedCell();
    if ( cell.col() != 1 )
	return;

    mDynamicCastGet(uiStratLevelSel*,levelsel,
	tbl_->getCellGroup(RowCol(cell.row(),2)))
    const bool havelvl = levelsel && levelsel->getID().isValid();
    if ( havelvl )
    {
	uiMSG().error( tr("Cannot change color of regional marker") );
	return;
    }

    OD::Color newcol = tbl_->getColor( cell );
    if ( selectColor(newcol,this,tr("Horizon Color")) )
	tbl_->setColor( cell, newcol );

    tbl_->setSelected( cell, false );
}

void lvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb)
    if ( !levelsel ) return;

    const OD::Color col = levelsel->getColor();
    if ( col == OD::Color::NoColor() ) return;

    const RowCol rc = tbl_->getCell( levelsel );
    tbl_->setColor( RowCol(rc.row(),1), col );
}

bool acceptOK( CallBacker* ) override
{
    for ( int idx=0; idx<objids_.size(); idx++ )
    {
	IOPar par;
	OD::Color col = tbl_->getColor( RowCol(idx,1) );
	par.set( sKey::Color(), col );

	mDynamicCastGet(uiStratLevelSel*,levelsel,
			tbl_->getCellGroup(RowCol(idx,2)))
	const Strat::LevelID lvlid =
			levelsel ? levelsel->getID() : Strat::LevelID::udf();
	IOPar displaypar;
	displaypar.set( sKey::StratRef(), lvlid );
	displaypar.set( sKey::Color(), col );
	EM::EMM().writeDisplayPars( objids_[idx], displaypar );
    }

    return true;
}


    uiTable*	tbl_;
    const TypeSet<MultiID>& objids_;

};


void uiSurfaceMan::stratSel( CallBacker* )
{
    const TypeSet<MultiID>& ids = selgrp_->getIOObjIds();
    uiSurfaceStratDlg dlg( this, ids );
    dlg.go();
}


class uiSurface2DMan : public uiDialog
{ mODTextTranslationClass(uiSurface2DMan)
public:

uiSurface2DMan( uiParent* p, const EM::IOObjInfo& info )
    :uiDialog(p,Setup(tr("2D Horizons management"),
	uiStrings::phrManage(EMHorizon2DTranslatorGroup::sTypeName(mPlural)),
	mODHelpKey(mSurface2DManHelpID)))
    , eminfo_(info)
{
    setCtrlStyle( CloseOnly );

    auto* topgrp = new uiGroup( this, "Top" );
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("2D lines"),
			 uiListBox::AboveMid );
    linelist_ = new uiListBox( topgrp, su, "linenames" );
    BufferStringSet linenames;
    info.getLineNames( linenames );
    linelist_->addItems( linenames );
    linelist_->selectionChanged.notify( mCB(this,uiSurface2DMan,lineSel) );

    auto* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    auto* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    lineSel( 0 );
}


void lineSel( CallBacker* )
{
    const int curitm = linelist_->currentItem();
    TypeSet< StepInterval<int> > trcranges;
    eminfo_.getTrcRanges( trcranges );

    BufferString txt;
    if ( trcranges.validIdx(curitm) )
    {
	StepInterval<int> trcrg = trcranges[ curitm ];
        txt += BufferString( sKey::FirstTrc(), ": " ); txt += trcrg.start_;
	txt += "\n";
        txt += BufferString( sKey::LastTrc(), ": " ); txt += trcrg.stop_;
	txt += "\n";
        txt += BufferString( "Trace Step: " ); txt += trcrg.step_;
    }

    infofld_->setText( txt );
}

    uiListBox*			linelist_;
    uiTextEdit*			infofld_;
    const EM::IOObjInfo&	eminfo_;

};


void uiSurfaceMan::man2dCB( CallBacker* )
{
    EM::IOObjInfo eminfo( curioobj_->key() );
    uiSurface2DMan dlg( this, eminfo );
    dlg.go();
}



class uiFltSetMan : public uiDialog
{ mODTextTranslationClass(uiFltSetMan)
public:
uiFltSetMan( uiParent* p, const IOObj& ioobj )
    :uiDialog(p,Setup(tr("FaultSet management"),
		      uiStrings::phrManage( uiStrings::sFault(mPlural)),
		      mODHelpKey(mFltSetManHelpID)))
    , ioobj_(ioobj)
    , dl_(ioobj.fullUserExpr(),File::DirListType::FilesInDir,"*.flt")
{
    setCtrlStyle( CloseOnly );

    auto* topgrp = new uiGroup( this, "Top" );
    uiListBox::Setup su( OD::ChooseAtLeastOne, uiStrings::sFault(mPlural),
			 uiListBox::AboveMid );
    fltlist_ = new uiListBox( topgrp, su, "inputfaults" );
    BufferStringSet fltnms;
    const int nrfaults = dl_.size();
    for ( int idx=0; idx<nrfaults; idx++ )
    {
	const FilePath fp( dl_.fullPath(idx) );
	const int id = toInt( fp.baseName(), mUdf(int) );
	BufferString fltnm( ioobj_.name() );
	fltnm.add( "_" ).add( id );
	fltnms.add( fltnm );
    }

    fltlist_->addItems( fltnms );
    fltlist_->selectionChanged.notify( mCB(this,uiFltSetMan,fltSel) );

    extractbut_ = new uiPushButton( topgrp, tr("Extract as Fault"),
				mCB(this,uiFltSetMan,extractCB), false );
    extractbut_->setIcon( "tree-flt" );
    extractbut_->attach( centeredBelow, fltlist_ );


    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    fltSel( 0 );
}

void fltSel( CallBacker* )
{
    const int curitm = fltlist_->currentItem();
    if ( curitm < 0 )
	return;

    const od_int64 filesz = File::getFileSize( dl_.fullPath(curitm) );
    BufferString txt( "Size on disk: " );
    txt.add( File::getFileSizeString(filesz) );

    infofld_->setText( txt );
}


void extractCB( CallBacker* )
{
    TypeSet<int> chosenfltidx;
    fltlist_->getChosen( chosenfltidx );
    const int nrflts = chosenfltidx.size();
    if (nrflts < 1 )
	return;

    const bool isbulk = nrflts > 1;

    BufferString fltnm( fltlist_->getText() );

    MultiID mid;

    if ( isbulk )
    {
	uiGenInputDlg dlg( this, tr("Extract Multiple Faults"),
							tr("Base Name") );
	//dlg.
	if ( !dlg.go() )
	    return;

	fltnm = dlg.text();
    }
    else
    {
	CtxtIOObj ctio( mIOObjContext( EMFault3D ) );
	ctio.ctxt_.forread_ = false;
	uiIOObjSelDlg dlg( this, ctio );
	dlg.selGrp()->getNameField()->setText( fltnm );

	dlg.setCaption( tr("Extract as Fault") );
	if ( !dlg.go() )
	    return;

	const IOObj* ioobj = dlg.ioObj();
	if ( !ioobj )
	    return;

	fltnm = ioobj->name();
	mid = ioobj->key();
    }

    ExecutorGroup savergroup( "Saving Faults" );

    for( int idx=0; idx< nrflts; idx++ )
    {
	BufferString outfltnm = fltnm;

	if ( isbulk )
	    outfltnm.add("_").add(idx);

	EM::ObjectID oid = EM::EMM().createObject( EM::Fault3D::typeStr(),
								    outfltnm );
	mDynamicCastGet( EM::Fault3D*, newflt, EM::EMM().getObject(oid) );

	FilePath fp( dl_.fullPath( chosenfltidx[idx] ) );

	EM::dgbSurfaceReader rdr( fp.fullPath(), outfltnm,
				  mTranslGroupName(EMFault3D) );
	rdr.setOutput( *newflt );
	if ( !rdr.execute() )
	    return;

	if ( !isbulk )
	    newflt->setMultiID( mid );

	savergroup.add( newflt->saver() );
    }

    savergroup.execute();
}

    uiListBox*			fltlist_;
    uiPushButton*		extractbut_;
    uiTextEdit*			infofld_;
    const IOObj&		ioobj_;
    DirList			dl_;

};


void uiSurfaceMan::manFltSetCB( CallBacker* )
{
    if ( !curioobj_ )
	return;

    uiFltSetMan dlg( this, *curioobj_ );
    dlg.go();
}


void uiSurfaceMan::copyFault2FaultSetCB( CallBacker* )
{
    uiFault2FaultSet dlg( this );
    if ( !dlg.go() )
	return;

    if ( type_ == EM::ObjectType::FltSet )
	updateCB( nullptr );
}
