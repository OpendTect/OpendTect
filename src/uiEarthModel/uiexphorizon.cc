/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiexphorizon.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uit2dconvsel.h"
#include "uiunitsel.h"
#include "od_ostream.h"
#include "od_helpids.h"
#include "uiioobjselgrp.h"
#include "transl.h"
#include "emioobjinfo.h"

#include <stdio.h> // for sprintf


static const char* zmodes[] = { sKey::Yes(), sKey::No(), "Transformed", 0 };
static const char* exptyps[] = { "X/Y", "Inl/Crl", "IESX (3d_ci7m)", 0 };
static const char* hdrtyps[] = { "No", "Single line", "Multi line", 0 };


#define mHdr1GFLineLen 102
#define mDataGFLineLen 148
#define mGFUndefValue 3.4028235E+38


mExpClass(uiEarthModel) write3DHorASCII : public Executor
{ mODTextTranslationClass(write3DHorASCII)
public:
			    write3DHorASCII(od_ostream&,const EM::Horizon3D*,
			    const ZAxisTransform* zatf,const UnitOfMeasure*,
			    const bool doxy,const bool addzpos,
			    const bool dogf,const bool issingle,
			    const int nrattrib,const int sectionidx,
			    const int sidx,BufferString udfstr,
			    BufferString dispstr);
    int			    nextStep();
    const char*		    message() const	   { return msg_; }
    const char*		    nrDoneText() const;
    od_int64		    nrDone() const;
    od_int64		    totalNr() const;

protected:
    od_ostream&		    stream_;
    const bool		    doxy_;
    const bool		    addzpos_;
    const bool		    dogf_;
    const int		    nrattrib_;
    const int		    sidx_;
    int			    maxsize_;
    BufferString	    udfstr_;
    const EM::Horizon3D*    hor_;
    const bool		    issingle_;
    EM::EMObjectIterator*   it_;
    const ZAxisTransform*   zatf_;
    const UnitOfMeasure*    unit_;
    BufferString	    msg_;
    int			    counter_;


};



write3DHorASCII::write3DHorASCII(od_ostream& stream,const EM::Horizon3D* hor,
      const ZAxisTransform* zatf, const UnitOfMeasure* unit, const bool doxy,
      const bool addzpos, const bool dogf, const bool issingle,
      const int nrattrib, const int sectionidx,const int sidx,
      BufferString udfstr, BufferString str )
      : Executor( str )
      , stream_(stream)
      , doxy_(doxy)
      , dogf_(dogf)
      , addzpos_(addzpos)
      , nrattrib_(nrattrib)
      , udfstr_(udfstr)
      , hor_(hor)
      , issingle_(issingle)
      , zatf_(zatf)
      , unit_(unit)
      , sidx_(sidx)
      , counter_(0)
{
    const EM::SectionID sectionid = hor->sectionID( sectionidx );
    it_ = hor->createIterator( sectionid );
    maxsize_ = it_->maximumSize();
}


od_int64 write3DHorASCII::nrDone() const
{
    return counter_ < maxsize_ ? counter_ : -1;
}


const char* write3DHorASCII::nrDoneText() const
{
    return "Number of points processed";
}


od_int64 write3DHorASCII::totalNr() const
{
    return maxsize_;
}


static void writeGF( od_ostream& strm, const BinID& bid, float z,
	      float val, const Coord& crd, int segid )
{
    char buf[mDataGFLineLen+2];
    const float crl = mCast( float, bid.crl() );
    const float gfval = (float) ( mIsUdf(val) ? mGFUndefValue : val );
    const float depth = (float) ( mIsUdf(z) ? mGFUndefValue : z );
    sprintf( buf, "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	  crd.x, crd.y, segid, 14, depth,
	  crl, crl, bid.crl(), gfval, bid.inl(),
	     "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


int write3DHorASCII::nextStep()
{
    BufferString str;
    const EM::PosID posid = it_->next();
    if ( posid.objectID()==-1 || counter_ > maxsize_ )
      return Executor::Finished();

    if ( !issingle_ )
      stream_ << hor_->name().quote('\"') << od_tab;

    Coord3 crd = hor_->getPos( posid );
    const BinID bid = SI().transform( crd.coord() );
    if ( zatf_ )
      crd.z = zatf_->transformTrc( bid, (float)crd.z );

    if ( zatf_ && SI().depthsInFeet() )
    {
	const UnitOfMeasure* uom = UoMR().get( "ft" );
	crd.z = uom->getSIValue( crd.z );
    }

    if ( !mIsUdf(crd.z) && unit_ )
      crd.z = unit_->userValue( crd.z );

    if ( dogf_ )
    {
      const float auxvalue = nrattrib_ > 0
	  ? hor_->auxdata.getAuxDataVal(0,posid) : mUdf(float);
      writeGF( stream_, bid, (float) crd.z, auxvalue,
		  crd.coord(), sidx_ );
      Executor::MoreToDo();
    }

    if ( !doxy_ )
    {
      stream_ << bid.inl() << od_tab << bid.crl();
    }
    else
    {
	// ostreams print doubles awfully
	str.setEmpty();
	str += crd.x; str += od_tab; str += crd.y;
      stream_ << str;
    }

    if ( addzpos_ )
    {
      if ( mIsUdf(crd.z) )
	  stream_ << od_tab << udfstr_;
      else
      {
	  str = od_tab; str += crd.z;
	  stream_ << str;
      }
    }

    for ( int idx=0; idx<nrattrib_; idx++ )
    {
      const float auxvalue = hor_->auxdata.getAuxDataVal( idx, posid );
      if ( mIsUdf(auxvalue) )
	  stream_ << od_tab << udfstr_;
	else
	{
	    str = od_tab; str += auxvalue;
	  stream_ << str;
      }
    }

    stream_ << od_newline;

    if ( dogf_ ) stream_ << "EOD";
    counter_++;
    stream_.flush();
    if ( stream_.isBad() )
    {
      msg_ =  "Cannot write output file";
      Executor::ErrorOccurred();
    }
    return Executor::MoreToDo();
}


// uiExportHorizon
uiExportHorizon::uiExportHorizon( uiParent* p, bool isbulk )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport(uiStrings::sHorizon()),
			mNoDlgTitle,mODHelpKey(mExportHorizonHelpID)))
    , infld_(0)
    , isbulk_(isbulk)
    , bulkinfld_(0)
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );
    setModal( false );
    setDeleteOnClose( false );
    IOObjContext ctxt = mIOObjContext( EMHorizon3D );
    uiIOObjSelGrp::Setup stup; stup.choicemode_ = OD::ChooseAtLeastOne;
    if ( !isbulk )
    {
	infld_ = new uiSurfaceRead( this, uiSurfaceRead::Setup(
				    EMHorizon3DTranslatorGroup::sGroupName())
		    .withsubsel(true).withsectionfld(false) );
	infld_->inpChange.notify( mCB(this,uiExportHorizon,inpSel) );
	infld_->attrSelChange.notify( mCB(this,uiExportHorizon,attrSel) );
    }
    else
	bulkinfld_ = new uiIOObjSelGrp( this, ctxt,
				      uiStrings::sHorizon(mPlural), stup );

    typfld_ = new uiGenInput( this, uiStrings::phrOutput( uiStrings::sType() ),
			      StringListInpSpec(exptyps) );
    typfld_->valuechanged.notify( mCB(this,uiExportHorizon,typChg) );
    if ( !isbulk )
	typfld_->attach( alignedBelow, infld_ );
    else
	typfld_->attach( alignedBelow, bulkinfld_ );

    settingsbutt_ = new uiPushButton( this, uiStrings::sSettings(),
				      mCB(this,uiExportHorizon,settingsCB),
				      false);
    settingsbutt_->attach( rightOf, typfld_ );

    zfld_ = new uiGenInput( this, uiStrings::phrOutput( toUiString("Z") ),
			    StringListInpSpec(zmodes) );
    zfld_->valuechanged.notify( mCB(this,uiExportHorizon,addZChg ) );
    zfld_->attach( alignedBelow, typfld_ );

    uiT2DConvSel::Setup su( 0, false );
    su.ist2d( SI().zIsTime() );
    transfld_ = new uiT2DConvSel( this, su );
    transfld_->display( false );
    transfld_->attach( alignedBelow, zfld_ );

    unitsel_ = new uiUnitSel( this, "Z Unit" );
    unitsel_->attach( alignedBelow, transfld_ );

    headerfld_ = new uiGenInput( this, tr("Header"),
                                 StringListInpSpec(hdrtyps) );
    headerfld_->attach( alignedBelow, unitsel_ );

    udffld_ = new uiGenInput( this, tr("Undefined value"),
			      StringInpSpec(sKey::FloatUdf()) );
    udffld_->attach( alignedBelow, headerfld_ );

    outfld_ = new uiFileInput( this,
	      uiStrings::sOutputASCIIFile(),
	      uiFileInput::Setup().forread(false) );
    outfld_->attach( alignedBelow, udffld_ );

    typChg( 0 );
    if ( !isbulk )
	inpSel( 0 );
}


uiExportHorizon::~uiExportHorizon()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }


static void initGF( od_ostream& strm, const char* hornm,
		    const char* comment )
{
    char gfbuf[mHdr1GFLineLen+2];
    gfbuf[mHdr1GFLineLen] = '\0';
    BufferString hnm( hornm );
    hnm.clean();
    sprintf( gfbuf, "PROFILE %17sTYPE 1  4 %45s3d_ci7m.ifdf     %s ms\n",
		    "", "", SI().xyInFeet() ? "ft" : "m " );
    int sz = hnm.size(); if ( sz > 17 ) sz = 17;
    OD::memCopy( gfbuf+8, hnm.buf(), sz );
    hnm = comment;
    sz = hnm.size(); if ( sz > 45 ) sz = 45;
    OD::memCopy( gfbuf+35, hnm.buf(), sz );
    strm << gfbuf << "SNAPPING PARAMETERS 5     0 1" << od_endl;
}


void uiExportHorizon::writeHeader( od_ostream& strm )
{
    if ( headerfld_->getIntValue() == 0 )
	return;

    BufferStringSet selattribs;
    if ( !isbulk_ && infld_->haveAttrSel() )
	infld_->getSelAttributes( selattribs );

    const bool doxy = typfld_->getIntValue() == 0;
    const bool addzpos = zfld_->getIntValue() != 1;
    if ( headerfld_->getIntValue() == 1 )
    {
	BufferString posstr = doxy ? "\"X\"\t\"Y\""
				   : "\"Inline\"\t\"Crossline\"";
	if ( addzpos ) posstr += "\t\"Z\"";

	for ( int idx=0; idx<selattribs.size(); idx++ )
	{
	    posstr += "\t\"";
	    posstr += selattribs.get( idx ); posstr += "\"";
	}

	strm << "# " << posstr.buf();
    }
    else
    {
	if ( doxy )
	    strm << "# 1: X\n# 2: Y";
	else
	    strm << "# 1: Inline\n# 2: Crossline";
	if ( addzpos )
	    strm << "\n# 3: Z";

	int colidx = addzpos ? 4 : 3;
	for ( int idx=0; idx<selattribs.size(); idx++ )
	{
	    strm << "\n# " << colidx << ": ";
	    strm << selattribs.get( idx );
	    colidx++;
	}
    }

    strm << "\n# - - - - - - - - - -\n";
}


bool uiExportHorizon::writeAscii()
{
    TypeSet<MultiID> midset;
    if ( !getInputMIDs(midset) )
	mErrRet(tr("Cannot find object in database"))
    ObjectSet<EM::IOObjInfo> emioinfoset;
    for ( int idx=0; idx<midset.size(); idx++ )
    {
	EM::IOObjInfo* emioinfo =  new EM::IOObjInfo(midset[idx]);
	emioinfoset.add(emioinfo);
    }
    const bool doxy = typfld_->getIntValue() == 0;
    const bool addzpos = zfld_->getIntValue() != 1;
    const bool dogf = typfld_->getIntValue() == 2;

    RefMan<ZAxisTransform> zatf = 0;
    if ( zfld_->getIntValue()==2 )
    {
	zatf = transfld_->getSelection();
	if ( !zatf )
	{
	 uiMSG().message(tr("Transform of selected option is not "
							    "implemented"));
	    return false;
	}
    }

    BufferString udfstr = udffld_->text();
    if ( udfstr.isEmpty() ) udfstr = sKey::FloatUdf();

    BufferString basename = outfld_->fileName();

    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    uiString errmsg;
    BufferString fname( basename );
    od_ostream stream( fname );
    for ( int horidx=0; horidx<emioinfoset.size(); horidx++ )
    {
	const IOObj* ioobj = !isbulk_ ? infld_->selIOObj() :
				      emioinfoset[horidx]->ioObj();
	if ( !ioobj ) mErrRet(uiStrings::phrCannotFind( tr("horizon object")));
	if ( !em.getSurfaceData(ioobj->key(),sd,errmsg) )
	    mErrRet( errmsg )

	EM::SurfaceIODataSelection sels( sd );
	if ( !isbulk_ )
	    infld_->getSelection( sels );
	sels.selvalues.erase();

	RefMan<EM::EMObject> emobj = em.createTempObject( ioobj->group() );
	if ( !emobj )
	    mErrRet(uiStrings::sCantCreateHor())

	emobj->setMultiID( ioobj->key() );
	mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr())
	PtrMan<Executor> loader = hor->geometry().loader( &sels );
	if ( !loader )
	    mErrRet( uiStrings::phrCannotRead( uiStrings::sHorizon() ) )

	uiTaskRunner taskrunner( this );
	if ( !TaskRunner::execute( &taskrunner, *loader ) ) return false;

	if ( !isbulk_ )
	    infld_->getSelection( sels );
	else
	    for( int idx=0; idx<sd.sections.size(); idx++ )
		sels.selsections += idx;
	if ( dogf && sels.selvalues.size() > 1 && uiMSG().askContinue(
			tr("Only the first selected attribute will be used\n"
				 "Do you wish to continue?")) )
	    return false;

	if ( !isbulk_ && !sels.selvalues.isEmpty() )
	{
	    ExecutorGroup exgrp( "Reading aux data" );
	    for ( int idx=0; idx<sels.selvalues.size(); idx++ )
		exgrp.add( hor->auxdata.auxDataLoader(sels.selvalues[idx]) );

	    if ( !TaskRunner::execute( &taskrunner, exgrp ) ) return false;
	}

	MouseCursorChanger cursorlock( MouseCursor::Wait );

	const UnitOfMeasure* unit = unitsel_->getUnit();
	TypeSet<int>& sections = sels.selsections;
	int zatvoi = -1;
	if ( zatf && zatf->needsVolumeOfInterest() ) //Get BBox
	{
	    TrcKeyZSampling bbox;
	    bool first = true;
	    for ( int sidx=0; sidx<sections.size(); sidx++ )
	    {
		const EM::SectionID sectionid = hor->sectionID(
						    sections[sidx] );
		PtrMan<EM::EMObjectIterator> it = hor->createIterator(
								sectionid );
		while ( true )
		{
		    const EM::PosID posid = it->next();
		    if ( posid.objectID()==-1 )
			break;

		    const Coord3 crd = hor->getPos( posid );
		    if ( !crd.isDefined() )
			continue;

		    const BinID bid = SI().transform( crd );
		    if ( first )
		    {
			first = false;
			bbox.hsamp_.start_ = bbox.hsamp_.stop_ = bid;
			bbox.zsamp_.start = bbox.zsamp_.stop = (float) crd.z;
		    }
		    else
		    {
			bbox.hsamp_.include( bid );
			bbox.zsamp_.include( (float) crd.z );
		    }
		}

	    }

	    if ( !first && zatf->needsVolumeOfInterest() )
	    {
		zatvoi = zatf->addVolumeOfInterest( bbox, false );
		if ( !zatf->loadDataIfMissing( zatvoi, &taskrunner ) )
		{
		    uiMSG().error( tr("Cannot load data for z-transform") );
		    return false;
		}
	    }
	}

	const int nrattribs = hor->auxdata.nrAuxData();
	for ( int sidx=0; sidx<sections.size(); sidx++ )
	{
	    BufferString dispstr("Writing Horizon ");
	    dispstr.add(hor->name());
	    ExecutorGroup exphorgrp( dispstr );
	    const int sectionidx = sections[sidx];

	    if ( stream.isBad() )
	    {
		mErrRet( uiStrings::sCantOpenOutpFile() );
	    }

	    if ( dogf )
		initGF( stream, gfname_.buf(), gfcomment_.buf() );
	    else
	    {
		stream.stdStream() << std::fixed;
		writeHeader( stream );
	    }

	    write3DHorASCII* executor = new write3DHorASCII(stream, hor,
	   zatf.ptr(), unit, doxy, addzpos, dogf, !isbulk_, nrattribs,
		sectionidx, sidx, udfstr, dispstr);
	    exphorgrp.add(executor);
	    if ( !TaskRunner::execute( &taskrunner, exphorgrp ) ) return false;

	if ( zatf && zatvoi>=0 )
	    zatf->removeVolumeOfInterest( zatvoi );
	}
}
    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( zfld_->getIntValue()==2 )
    {
	if ( !transfld_->acceptOK() )
	    return false;
    }

    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

    if ( File::exists(outfnm) &&
	 !uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()) )
	return false;

    const bool res = writeAscii();
    if ( !res )
    {
      uiMSG().error( uiStrings::phrCannotWrite(tr("output file.")));
      return false;
    }
    uiString msg = tr("%1 successfully exported."
			"\nDo you want to export more %1").arg(
				  uiStrings::sHorizon(!isbulk_?1:mPlural));
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


bool uiExportHorizon::getInputMIDs( TypeSet<MultiID>& midset )
{
    if ( !isbulk_ )
    {
	const IOObj* ioobj = infld_->selIOObj();
	if ( !ioobj ) return false;

	MultiID mid = ioobj->key();
	midset.add(mid);
    }
    else
	bulkinfld_->getChosen( midset );

    return true;
}


void uiExportHorizon::typChg( CallBacker* cb )
{
    attrSel( cb );
    addZChg( cb );

    const bool isgf = typfld_->getIntValue() == 2;
    headerfld_->display( !isgf );
    if ( isgf && gfname_.isEmpty() )
	settingsCB( cb );
}


void uiExportHorizon::inpSel( CallBacker* )
{
    const IOObj* ioobj = infld_ ? infld_->selIOObj() : 0;
    if ( ioobj )
	gfname_ = ioobj->name();
}


void uiExportHorizon::addZChg( CallBacker* )
{
    const bool isgf = typfld_->getIntValue() == 2;
    settingsbutt_->display( isgf );
    zfld_->display( !isgf );
    transfld_->display( !isgf && zfld_->getIntValue()==2 );

    const bool displayunit = !isgf && zfld_->getIntValue()!=1;
    if ( displayunit )
    {
	FixedString zdomain = getZDomain();
	if ( zdomain==ZDomain::sKeyDepth() )
	    unitsel_->setPropType( PropertyRef::Dist );
	else if ( zdomain==ZDomain::sKeyTime() )
	{
	    unitsel_->setPropType( PropertyRef::Time );
	    unitsel_->setUnit( "Milliseconds" );
	}
    }

    unitsel_->display( displayunit );
}


FixedString uiExportHorizon::getZDomain() const
{
    FixedString zdomain = ZDomain::SI().key();

    if ( typfld_->getIntValue()==2 || zfld_->getIntValue()==2 )
    {
	zdomain = transfld_->selectedToDomain();
    }

    return zdomain;
}


void uiExportHorizon::attrSel( CallBacker* )
{
    const bool isgf = typfld_->getIntValue() == 2;
    udffld_->display( !isgf && infld_ && infld_->haveAttrSel() );
}


void uiExportHorizon::settingsCB( CallBacker* )
{
    if ( typfld_->getIntValue() != 2 )
	return;

    uiDialog dlg( this, uiDialog::Setup(tr("IESX details"),
					mNoDlgTitle,mNoHelpKey) );
    uiGenInput* namefld = new uiGenInput( &dlg, tr("Horizon name in file") );
    uiGenInput* commentfld = new uiGenInput( &dlg, tr("[Comment]") );
    commentfld->attach( alignedBelow, namefld );
    namefld->setText( gfname_.buf() );
    commentfld->setText( gfcomment_.buf() );

    while ( dlg.go() )
    {
	FixedString nm = namefld->text();
	if ( nm.isEmpty() )
	{
	    uiMSG().error( tr("No name selected") );
	    continue;
	}

	gfname_ = namefld->text();
	gfcomment_ = commentfld->text();
	return;
    }
}
