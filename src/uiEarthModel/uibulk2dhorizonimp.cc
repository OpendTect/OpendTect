/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		July 2017
________________________________________________________________________

-*/


#include "uibulk2dhorizonimp.h"

#include "binidvalset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "executor.h"
#include "posinfodetector.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"
#include "uifileinput.h"
#include "emhorizon2d.h"
#include "survgeom2d.h"
#include "dbman.h"
#include "emsurfacetr.h"
#include "file.h"
#include "ui2dhorimporter.h"
#include "randcolor.h"


class BulkHorizon2DAscIO : public Table::AscIO
{ mODTextTranslationClass(BulkHorizon2DAscIO)
public:
BulkHorizon2DAscIO( Table::FormatDesc& fd, BufferString fnm )
    : Table::AscIO(fd)
    , strm_(*new od_istream(fnm))
    , finishedreadingheader_(false)
    , fnm_(fnm)
{
    totalLine();
}


static Table::FormatDesc* getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Bulk 2D Horizon" );

    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Horizon name",
							    Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Line name", Table::Required );
    Table::TargetInfo* ti = new Table::TargetInfo( "Position", DoubleInpSpec(),
					    Table::Optional );
    ti->form(0).add( DoubleInpSpec() ); ti->form(0).setName( "X Y" );
    fd->bodyinfos_ += ti;
    Table::TargetInfo* trcspti = new Table::TargetInfo( "", Table::Required );
    trcspti->form(0).setName( "Trace Nr" );
    Table::TargetInfo::Form* spform =
		    new Table::TargetInfo::Form( "ShotPt Nr", IntInpSpec() );
    trcspti->add( spform );
    fd->bodyinfos_ += trcspti;
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    return fd;
}


bool isTrNr() const
{ return formOf( false, 3 ) == 0; }


bool getLine( BufferString& hornm, BufferString& linenm, Coord3& crd,
								int& trcnr )
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    int idx = 0;
    hornm = text( idx );
    linenm = text(1);
    crd.x_ = getDValue( 2, udfval_ );
    crd.y_ = getDValue( 3, udfval_ );
    crd.z_ = getFValue( 5, udfval_ );
    trcnr = mCast(int,getDValue(4, udfval_));
    return true;
}


od_int64 totalLine()
{
    od_istream strm(fnm_);
    if ( totalnr_ > 0 ) return totalnr_;

    totalnr_ = 0;
    if ( !strm.isOK() ) return 0;

    BufferString line;
    while ( strm.isOK() )
	{ strm.getLine( line ); totalnr_++; }

    totalnr_ -= fd_.nrhdrlines_;

    return totalnr_;
}


    od_istream&			strm_;
    float			udfval_;
    bool			finishedreadingheader_;
    od_int64			totalnr_;
    BufferString		fnm_;


};


class BulkDataReader : public Executor
{ mODTextTranslationClass(BulkDataReader)
public:
BulkDataReader( BulkHorizon2DAscIO* ascio )
    : Executor("Importing Data")
    , ascio_(ascio)
    , nrdone_(0)
    , bidvs_(0)
{
    totalnr_ = ascio_->totalnr_;
}

int nextStep()
{
    if ( nrdone_ == totalnr_ )
	return Executor::Finished();
    BufferString hornm;
    BufferString linenm;
    Coord3 crd;
    int trcnr=0;

    if ( ascio_->getLine(hornm,linenm,crd,trcnr) )
    {
	if ( prevhornm_.isEmpty() )
	    prevhornm_ = hornm;

	if ( !hornm.isEmpty() )
	    hornms_.addIfNew(hornm);

	if ( !linenm.isEmpty() )
	    linenms_.addIfNew(linenm);
	BufferString bsr;
	for ( int lidx=0; lidx<linenms_.size(); lidx++ )
	    bsr = linenms_.get(lidx);

	if ( !bidvs_ || (prevhornm_ != hornm) )
	    bidvs_ = new BinIDValueSet( 1, false );

	if ( !crd.isDefined() )
	    Executor::MoreToDo();

	Pos::GeomID geomid = Survey::GM().getGeomID(linenm);
	int nr = 0;
	const Survey::Geometry2D* curlinegeom(0);


	if ( ascio_->isTrNr() )
	    nr = trcnr;
	else if ( crd.isDefined() )
	{
	    mDynamicCast( const Survey::Geometry2D*, curlinegeom,
					Survey::GM().getGeometry(geomid) );
	    if ( !curlinegeom )
		return Executor::ErrorOccurred();
	    PosInfo::Line2DPos pos;
	    if ( !curlinegeom->data().getPos(crd.getXY(),pos,
							SI().inlDistance()) )
		Executor::MoreToDo();
	    nr = pos.nr_;
	}

	BinID bid(geomid,nr);
	bidvs_->add( bid, crd.z_ );
	if ( (prevhornm_ != hornm) || data_.isEmpty() )
	{
	    prevhornm_ = hornm;
	    data_ += bidvs_;
	}
    }
    nrdone_++;
    return Executor::MoreToDo();
}

uiString message() const
{ return tr("Horizon Import"); }



uiString nrDoneText() const
{ return tr("Positions written:"); }

od_int64 nrDone() const
{ return nrdone_; }

od_int64 totalNr() const
{
    return ascio_->totalnr_;
}
    od_int64			nrdone_;
    od_int64			totalnr_;
    BufferStringSet		hornms_;
    BufferStringSet		linenms_;
    BinIDValueSet*		bidvs_;
    ObjectSet<BinIDValueSet>	data_;
    BulkHorizon2DAscIO*		ascio_;
    BufferString		prevhornm_;
};




uiBulk2DHorizonImport::uiBulk2DHorizonImport( uiParent* p, bool isbulk )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrImport(
				 tr("Multiple 2D Horizons")), mNoDlgTitle,
				 mODHelpKey(mBulkHorizonImportHelpID) )
			    .modal(false))
    , fd_(BulkHorizon2DAscIO::getDesc())
    , isbulk_(isbulk)
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiFileSel( this,
		      uiStrings::sInputASCIIFile(),
		      uiFileSel::Setup().withexamine(true)
		      .examstyle(File::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				    mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfld_ );
    BufferStringSet udftreatments;
    udftreatments.add( "Skip" ).add( "Adopt" ).add( "Interpolate" );
    udftreatfld_ = new uiGenInput( this, tr("Undefined values"),
				   StringListInpSpec(udftreatments) );
    udftreatfld_->attach( alignedBelow, dataselfld_ );
}


uiBulk2DHorizonImport::~uiBulk2DHorizonImport()
{
    delete &fd_;
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }


#define mDeburstRet( retval, unreffn ) \
{ \
    for ( int horidx=0; horidx<hor2ds.size(); horidx++ ) \
    { \
	if ( hor2ds[horidx]->hasBurstAlert() ) \
	    hor2ds[horidx]->setBurstAlert( false ); \
\
	hor2ds[horidx]->unreffn(); \
    } \
    return retval; \
 } \


bool uiBulk2DHorizonImport::acceptOK()
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("the input file name")) )
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpen(uiStrings::sInputFile().toLower()))

    if ( !dataselfld_->commit() )
	return false;


    BulkHorizon2DAscIO ascio( *fd_, fnm );

    BulkDataReader* reader = new BulkDataReader(&ascio);
    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute(&taskrunner,*reader) )
	return false;
    BufferStringSet hornms = reader->hornms_;
    BufferStringSet linenmset = reader->linenms_;
    ObjectSet<BinIDValueSet> data = reader->data_;
    ObjectSet<EM::Horizon2D> hor2ds;
    EM::EMManager& em = EM::EMM();
    PtrMan<IOObj> existioobj(0);
    BufferStringSet existinghornms;
    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	existioobj = DBM().getByName( IOObjContext::Surf, hornms.get(idx) );
	if ( existioobj )
	    existinghornms.add(hornms.get(idx));
    }

    if ( !existinghornms.isEmpty() )
    {
	bool ret = uiMSG().askGoOn(tr("Horizons %1 already exist. "
	    "Do You want to overwrite them.")
	    .arg(toUiString(existinghornms.getDispString(5))));
	if ( !ret )
	    return false;
    }

    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	hor2ds.setEmpty();
	BufferString hornm = hornms.get( idx );
	PtrMan<IOObj> ioobj = DBM().getByName( hornm,
				EMHorizon2DTranslatorGroup::sGroupName() );

	EM::ObjectID id = ioobj ? em.getObjectID( ioobj->key() ) : -1;
	EM::EMObject* emobj = em.getObject(id);
	if ( emobj )
	    emobj->setBurstAlert( true );

	PtrMan<Executor> exec = ioobj ? em.objectLoader( ioobj->key() ) : 0;

	id = em.createObject( EM::Horizon2D::typeStr(), hornm );
	mDynamicCastGet(EM::Horizon2D*,hor,em.getObject(id));
	if ( ioobj )
	    hor->setDBKey( ioobj->key() );

	hor->setPreferredColor(getRandomColor());
	hor->ref();
	hor->setBurstAlert( true );
	hor2ds += hor;

	hor2ds.add(hor);

	PtrMan<Horizon2DBulkImporter> imprtr =
	new Horizon2DBulkImporter( linenmset, hor2ds, data[idx],
	    (Horizon2DBulkImporter::UndefTreat) udftreatfld_->getIntValue() );
	uiTaskRunner impdlg( this );
	if ( !TaskRunner::execute(&impdlg,*imprtr) )
	    mDeburstRet( false, unRef );

	PtrMan<Executor> saver = hor2ds[0]->saver();
	if ( !saver->execute() )
	    mErrRet(uiStrings::phrCannotSave(toUiString(hornms.get(idx))))
    }
    uiString msg = tr( "2D Horizons successfully imported.\n\n"
		    "Do you want to export more 2DHorizons?" );
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}
