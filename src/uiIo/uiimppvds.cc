/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimppvds.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"

#include "ctxtioobj.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "od_istream.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "statrand.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"


uiImpPVDS::uiImpPVDS( uiParent* p, bool is2d )
    : uiDialog(p,Setup(tr("Import Cross-plot Data"),mODHelpKey(mImpPVDSHelpID))
		    .modal(false))
    , fd_(*new Table::FormatDesc("Cross-plot data"))
    , is2d_(is2d)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::ViewStyle::Table );
    mAttachCB( inpfld_->valueChanged, uiImpPVDS::inputChgd );

    fd_.bodyinfos_ += Table::TargetInfo::mkHorPosition( false );
    fd_.bodyinfos_ += Table::TargetInfo::mkZPosition( false );
    if ( is2d_ )
	fd_.bodyinfos_ += new Table::TargetInfo("Trace number", IntInpSpec() );
    dataselfld_ = new uiTableImpDataSel( this, fd_,
		  mODHelpKey(mTableImpDataSelpvdsHelpID)  );
    dataselfld_->attach( alignedBelow, inpfld_ );

    row1isdatafld_ = new uiGenInput( this, tr("First row contains"),
			BoolInpSpec(false,tr("Data"),tr("Column names")) );
    row1isdatafld_->attach( alignedBelow, dataselfld_ );

    IOObjContext ctxt( mIOObjContext(PosVecDataSet) );
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt, tr("Output Cross-plot data") );
    outfld_->attach( alignedBelow, row1isdatafld_ );
}


uiImpPVDS::~uiImpPVDS()
{
    detachAllNotifiers();
    delete &fd_;
}


void uiImpPVDS::inputChgd( CallBacker* )
{
    const FilePath fnmfp( inpfld_->fileName() );
    outfld_->setInputText( fnmfp.baseName() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiImpPVDS::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() || File::isEmpty(fnm) )
	mErrRet(tr("Please select an existing input file"))
    if ( !dataselfld_->commit() )
	return false;

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	return false;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet(uiStrings::sCantOpenInpFile())

    RefMan<DataPointSet> dps = new DataPointSet( is2d_ );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    bool rv = getData( strm, fd_, *dps );
    MouseCursorManager::restoreOverride();
    if ( !rv || !writeData(*dps,*ioobj) )
	return false;

    uiString msg = tr("Cross-plot Data successfully imported."
		      "\n\nDo you want to import more data?");
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


class uiImpPVDSAscio : public Table::AscIO
{
public:
uiImpPVDSAscio( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
    , rownr_(-1)
{
    needfullline_ = true;
    atend_ = !getHdrVals( strm_ );
    inlgen_.start_ = SI().inlRange(true).start_;
    inlgen_.step_ = SI().inlRange(true).stop_ - inlgen_.start_;
    crlgen_.start_ = SI().crlRange(true).start_;
    crlgen_.step_ = SI().crlRange(true).stop_ - crlgen_.start_;
    zgen_.start_ = SI().zRange(true).start_;
    zgen_.step_ = SI().zRange(true).stop_ - zgen_.start_;
}

bool isXY() const
{
    const Table::TargetInfo* posinfo = fd_.bodyinfos_[0];
    return !posinfo || posinfo->selection_.form_ == 0;
}


bool getLine()
{
    if ( atend_ ) return false;

    atend_ = true;
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;
    atend_ = false;

    if ( rownr_ == -1 )
    {
	for ( int icol=0; icol<fullline_.size(); icol++ )
	{
	    if ( !fd_.bodyUsesCol(icol) )
	    {
		BufferString dcnm( havecolnms_ ? fullline_.get(icol)
					       : BufferString("Col ",icol+1) );
		if ( dcnm.isEmpty() )
		    continue;

		datacolnms_ += new BufferString( dcnm );
		datacols_ += icol;
	    }
	}
	rownr_++;
	if ( havecolnms_ )
	    return getLine();
    }

    if ( fd_.bodyinfos_[0]->selection_.isInFile(0) &&
	 fd_.bodyinfos_[0]->selection_.isInFile(1) )
    {
	if ( isXY() )
	    coord_ = getPos( 0, 1 );
	else
	    coord_ = SI().transform( getBinID(0,1) );
    }
    else
    {
	double finl = double(inlgen_.start_) + gen_.get() * inlgen_.step_;
	double fcrl = double(inlgen_.start_) + gen_.get() * inlgen_.step_;
	coord_ = SI().binID2Coord().transform( Coord(finl,fcrl) );
    }

    if ( fd_.bodyinfos_[1]->selection_.isInFile() )
	z_ = getFValue( 2 );
    else
	z_ = (float) ( zgen_.start_ + gen_.get() * zgen_.step_ );

    if ( is2d_ && fd_.bodyinfos_[2]->selection_.isInFile() )
	trcnr_ = getIntValue( 3 );
    else
	trcnr_ = rownr_ + 1;

    datavals_.erase();
    for ( int icol=0; icol<datacols_.size(); icol++ )
    {
	const int linecol = datacols_[icol];
	datavals_ += linecol < fullline_.size()
		   ? fullline_.get( datacols_[icol] ).toFloat() : mUdf(float);
    }

    rownr_++;
    return true;
}

    od_istream&		strm_;
    bool		havecolnms_;
    bool		is2d_;

    bool		atend_;
    Coord		coord_;
    float		z_;
    int			trcnr_;

    int			rownr_;
    SamplingData<double> inlgen_;
    SamplingData<double> crlgen_;
    SamplingData<float>	zgen_;
    Stats::RandGen	gen_;

    TypeSet<int>	datacols_;
    BufferStringSet	datacolnms_;
    TypeSet<float>	datavals_;

};


bool uiImpPVDS::getData( od_istream& strm, Table::FormatDesc& fd,
			 DataPointSet& dps )
{
    uiImpPVDSAscio aio( fd, strm );
    aio.havecolnms_ = !row1isdatafld_->getBoolValue();
    aio.is2d_ = is2d_;
    if ( aio.atend_ )
	mErrRet(aio.errMsg());

    if ( !aio.getLine() )
	mErrRet(tr("No data found in file"));

    for ( int idx=0; idx<aio.datacolnms_.size(); idx++ )
	dps.dataSet().add( new DataColDef(aio.datacolnms_.get(idx)) );

    while ( true )
    {
	if ( aio.coord_.isDefined() )
	{
	    DataPointSet::Pos dpspos( aio.coord_, aio.z_ );
	    DataPointSet::DataRow dr( dpspos );
	    dr.data_ = aio.datavals_;
	    dps.addRow( dr );
	}

	if ( !aio.getLine() )
	    break;
    }

    BufferStringSet colnms;
    colnms.add( sKey::Inline() ).add( sKey::Crossline() )
	  .add( sKey::XCoord() ).add( sKey::YCoord() );
    for ( int idx=0; idx<dps.dataSet().nrCols(); idx++ )
    {
	const BufferString& cdnm = dps.dataSet().colDef(idx).name_;
	if ( !colnms.isPresent(cdnm.buf(),OD::CaseInsensitive) )
	    continue;

	dps.dataSet().removeColumn( idx );
	idx--;
    }

    dps.dataChanged();
    return true;
}


bool uiImpPVDS::writeData( const DataPointSet& dps, const IOObj& ioobj )
{
    if ( dps.isEmpty() )
	mErrRet(tr("No data read"))

    uiString errmsg;
    MouseCursorManager::setOverride( MouseCursor::Wait );
    const bool isok = dps.dataSet().putTo( ioobj.fullUserExpr(false), errmsg,
					   false);
    MouseCursorManager::restoreOverride();
    if ( !isok )
	mErrRet( errmsg )

    return true;
}
