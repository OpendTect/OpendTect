/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiimppvds.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "posvecdatasettr.h"
#include "ioobj.h"
#include "file.h"
#include "strmprov.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "statrand.h"
#include "survinfo.h"
#include "rcol2coord.h"
#include "tabledef.h"
#include "tableascio.h"


uiImpPVDS::uiImpPVDS( uiParent* p, bool is2d )
    : uiDialog(p,uiDialog::Setup("Import cross-plot data",
				 "Import column data for cross-plots",
				 "111.0.7"))
    , fd_(*new Table::FormatDesc("Cross-plot data"))
    , is2d_(is2d)
{
    uiFileInput::Setup su( uiFileDialog::Txt );
    su.withexamine(true).examstyle(uiFileInput::Setup::Table).forread(true);
    inpfld_ = new uiFileInput( this, "Input file", su );

    fd_.bodyinfos_ += Table::TargetInfo::mkHorPosition( false );
    fd_.bodyinfos_ += Table::TargetInfo::mkZPosition( false );
    if ( is2d_ )
	fd_.bodyinfos_ += new Table::TargetInfo( "Trace number", IntInpSpec() );
    dataselfld_ = new uiTableImpDataSel( this, fd_, "111.0.8" );
    dataselfld_->attach( alignedBelow, inpfld_ );

    row1isdatafld_ = new uiGenInput( this, "First row contains",
	    			BoolInpSpec(false,"Data","Column names") );
    row1isdatafld_->attach( alignedBelow, dataselfld_ );

    IOObjContext ctxt( mIOObjContext(PosVecDataSet) );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt, "Output data set" );
    outfld_->attach( alignedBelow, row1isdatafld_ );
}


uiImpPVDS::~uiImpPVDS()
{
    delete &fd_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiImpPVDS::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() || File::isEmpty(fnm) )
	mErrRet("Please select an existing input file")
    if ( !dataselfld_->commit() )
	return false;
    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	return false;
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet("Cannot open input file")

    DataPointSet dps( is2d_ );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    bool rv = getData( *sd.istrm, fd_, dps );
    sd.close();
    MouseCursorManager::restoreOverride();

    return rv ? writeData( dps, *ioobj ) : false;
}


class uiImpPVDSAscio : public Table::AscIO
{
public:
uiImpPVDSAscio( const Table::FormatDesc& fd, std::istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
    , rownr_(-1)
{
    needfullline_ = true;
    atend_ = !getHdrVals( strm_ );
    inlgen_.start = SI().inlRange(true).start;
    inlgen_.step = SI().inlRange(true).stop - inlgen_.start;
    crlgen_.start = SI().crlRange(true).start;
    crlgen_.step = SI().crlRange(true).stop - crlgen_.start;
    zgen_.start = SI().zRange(true).start;
    zgen_.step = SI().zRange(true).stop - zgen_.start;
    Stats::randGen().init();
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
    {
	coord_.x = getdValue( 0 ); coord_.y = getdValue( 1 );
	if ( formOf(false,0) == 1 )
	    coord_ = SI().transform( BinID(mNINT32(coord_.x),mNINT32(coord_.y)) );
    }
    else
    {
	double finl = inlgen_.start + Stats::randGen().get() * inlgen_.step;
	double fcrl = inlgen_.start + Stats::randGen().get() * inlgen_.step;
	coord_ = SI().binID2Coord().transform( Coord(finl,fcrl) );
    }
    if ( fd_.bodyinfos_[1]->selection_.isInFile() )
	z_ = getfValue( 2 );
    else
	z_ = (float) ( zgen_.start + Stats::randGen().get() * zgen_.step );
    if ( is2d_ && fd_.bodyinfos_[2]->selection_.isInFile() )
	trcnr_ = getIntValue( 3 );
    else
	trcnr_ = rownr_ + 1;

    datavals_.erase();
    for ( int icol=0; icol<datacols_.size(); icol++ )
    {
	int linecol = datacols_[icol];
	if ( linecol >= fullline_.size() )
	    datavals_ += mUdf(float);
	else
	{
	    float val = toFloat( fullline_.get( datacols_[icol] ).buf() );
	    datavals_ += val;
	}
    }

    rownr_++;
    return true;
}

    std::istream&	strm_;
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


bool uiImpPVDS::getData( std::istream& strm, Table::FormatDesc& fd,
			 DataPointSet& dps )
{
    uiImpPVDSAscio aio( fd, strm );
    aio.havecolnms_ = !row1isdatafld_->getBoolValue();
    aio.is2d_ = is2d_;
    if ( aio.atend_ )
	mErrRet(aio.errMsg());

    if ( !aio.getLine() )
	mErrRet("No data found in file");

    for ( int idx=0; idx<aio.datacolnms_.size(); idx++ )
	dps.dataSet().add( new DataColDef(aio.datacolnms_.get(idx)) );

    while ( true )
    {
	DataPointSet::Pos dpspos( aio.coord_, aio.z_ );
	if ( is2d_ ) dpspos.nr_ = aio.trcnr_;
	DataPointSet::DataRow dr( dpspos );
	dr.data_ = aio.datavals_;
	dps.addRow( dr );

	if ( !aio.getLine() )
	    break;
    }

    dps.dataChanged();
    return true;
}


bool uiImpPVDS::writeData( const DataPointSet& dps, const IOObj& ioobj )
{
    if ( dps.isEmpty() )
	mErrRet("No data read")

    BufferString errmsg;
    MouseCursorManager::setOverride( MouseCursor::Wait );
    const bool isok = dps.dataSet().putTo( ioobj.fullUserExpr(false), errmsg,
	    				   false);
    MouseCursorManager::restoreOverride();
    if ( !isok )
	mErrRet(errmsg)

    return true;
}
