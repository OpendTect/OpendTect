/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/


#include "uiimppvds.h"

#include "uigeninput.h"
#include "uifilesel.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "uimsg.h"
#include "uistrings.h"

#include "ioobjctxt.h"
#include "posvecdatasettr.h"
#include "ioobj.h"
#include "file.h"
#include "filepath.h"
#include "od_istream.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "statrand.h"
#include "survinfo.h"
#include "tabledef.h"
#include "tableascio.h"
#include "od_helpids.h"


uiImpPVDS::uiImpPVDS( uiParent* p, bool is2d )
    : uiDialog(p,uiDialog::Setup(tr("Import Cross-plot Data"),
				 mNoDlgTitle, mODHelpKey(mImpPVDSHelpID))
			   .modal(false))
    , fd_(*new Table::FormatDesc("Cross-plot data"))
    , is2d_(is2d)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiFileSel::Setup su( OD::TextContent );
    su.withexamine( true ).examstyle( File::Table );
    inpfld_ = new uiFileSel( this, uiStrings::phrInput(uiStrings::sFile()), su);
    mAttachCB( inpfld_->newSelection, uiImpPVDS::inpChgd );

    fd_.bodyinfos_ += Table::TargetInfo::mkHorPosition( true, false, false );
    fd_.bodyinfos_ += Table::TargetInfo::mkZPosition( false );
    if ( is2d_ )
	fd_.bodyinfos_ += new Table::TargetInfo( uiStrings::sTraceNumber(),
						    IntInpSpec() );
    dataselfld_ = new uiTableImpDataSel( this, fd_,
		  mODHelpKey(mTableImpDataSelpvdsHelpID)  );
    dataselfld_->attach( alignedBelow, inpfld_ );

    row1isdatafld_ = new uiGenInput( this, tr("First row contains"),
			BoolInpSpec(false,tr("Data"),tr("Column names")) );
    row1isdatafld_->attach( alignedBelow, dataselfld_ );

    IOObjContext ctxt( mIOObjContext(PosVecDataSet) );
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt, uiStrings::phrOutput(
					    uiStrings::sDataSet()));
    outfld_->attach( alignedBelow, row1isdatafld_ );
}


uiImpPVDS::~uiImpPVDS()
{
    detachAllNotifiers();
    delete &fd_;
}


void uiImpPVDS::inpChgd( CallBacker* )
{
    const File::Path fp( inpfld_->fileName() );
    outfld_->setInputText( fp.baseName() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiImpPVDS::acceptOK()
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() || File::isEmpty(fnm) )
	mErrRet(uiStrings::phrSelect(tr("an existing input file")))
    if ( !dataselfld_->commit() )
	return false;

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	return false;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpenInpFile())

    uiUserShowWait usw( this, uiStrings::sCollectingData() );
    RefMan<DataPointSet> dps = new DataPointSet( is2d_ );
    if ( !getData( strm, fd_, *dps ) )
	return false;

    usw.setMessage( uiStrings::sSavingData() );
    if ( !writeData(*dps,*ioobj) )
	return false;

    uiMSG().message( tr("Cross-plot Data successfully imported.") );
    return false;
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
    inlgen_.start = SI().inlRange().start;
    inlgen_.step = SI().inlRange().stop - inlgen_.start;
    crlgen_.start = SI().crlRange().start;
    crlgen_.step = SI().crlRange().stop - crlgen_.start;
    zgen_.start = SI().zRange(OD::UsrWork).start;
    zgen_.step = SI().zRange(OD::UsrWork).stop - zgen_.start;
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

    if ( fd_.bodyinfos_[0]->selection_.isInFile(0)
      && fd_.bodyinfos_[0]->selection_.isInFile(1))
	coord_ = getPos( 0 , 1 );
    else
    {
	double finl = inlgen_.start + Stats::randGen().get() * inlgen_.step;
	double fcrl = inlgen_.start + Stats::randGen().get() * inlgen_.step;
	coord_ = SI().binID2Coord().transform( Coord(finl,fcrl) );
    }

    if ( fd_.bodyinfos_[1]->selection_.isInFile() )
	z_ = getFValue( 2 );
    else
	z_ = (float) ( zgen_.start + Stats::randGen().get() * zgen_.step );

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
	    if ( is2d_ )
	    {
		Bin2D b2d( dpspos.bin2D() );
		b2d.trcNr() = aio.trcnr_;
		dpspos.set( b2d );
	    }
	    DataPointSet::DataRow dr( dpspos );
	    dr.data_ = aio.datavals_;
	    dps.addRow( dr );
	}

	if ( !aio.getLine() )
	    break;
    }

    dps.dataChanged();
    return true;
}


bool uiImpPVDS::writeData( const DataPointSet& dps, const IOObj& ioobj )
{
    if ( dps.isEmpty() )
	mErrRet(tr("No data read"))

    uiString errmsg;
    const bool isok = dps.dataSet().putTo( ioobj.fullUserExpr(false), errmsg,
					   false);
    if ( !isok )
	mErrRet(errmsg)

    return true;
}
