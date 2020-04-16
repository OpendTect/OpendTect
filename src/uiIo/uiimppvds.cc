/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiimppvds.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"

#include "ctxtioobj.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "statrand.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"


uiImpPVDS::uiImpPVDS( uiParent* p, bool is2d )
    : uiDialog(p,uiDialog::Setup(tr("Import Cross-plot Data"),
				 mNoDlgTitle, mODHelpKey(mImpPVDSHelpID))
			   .modal(false))
    , fd_(*new Table::FormatDesc("Cross-plot data"))
    , is2d_(is2d)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiFileInput::Setup su( uiFileDialog::Txt );
    su.withexamine(true).examstyle(File::Table).forread(true);
    inpfld_ = new uiFileInput( this, uiStrings::sInputFile(), su );
    mAttachCB( inpfld_->valuechanged, uiImpPVDS::inputChgd );

    fd_.bodyinfos_ += Table::TargetInfo::mkHorPosition( false );
    fd_.bodyinfos_ += Table::TargetInfo::mkZPosition( false );
    if ( is2d_ )
	fd_.bodyinfos_ += new Table::TargetInfo("Trace number", IntInpSpec() );
    dataselfld_ = new uiTableImpDataSel( this, fd_,
		  mODHelpKey(mTableImpDataSelpvdsHelpID)  );
    dataselfld_->attach( alignedBelow, inpfld_ );

    row1isdatafld_ = new uiGenInput( this, tr("First row contains"),
				BoolInpSpec(false,tr("Data","Column names")) );
    row1isdatafld_->attach( alignedBelow, dataselfld_ );

    IOObjContext ctxt( mIOObjContext(PosVecDataSet) );
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt, uiStrings::phrOutput(
					uiStrings::phrData(uiStrings::sSet())));
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

    DataPointSet dps( is2d_ );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    bool rv = getData( strm, fd_, dps );
    MouseCursorManager::restoreOverride();

    return rv ? writeData( dps, *ioobj ) : false;
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
    inlgen_.start = SI().inlRange(true).start;
    inlgen_.step = SI().inlRange(true).stop - inlgen_.start;
    crlgen_.start = SI().crlRange(true).start;
    crlgen_.step = SI().crlRange(true).stop - crlgen_.start;
    zgen_.start = SI().zRange(true).start;
    zgen_.step = SI().zRange(true).stop - zgen_.start;
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
	    if ( is2d_ ) dpspos.nr_ = aio.trcnr_;
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

    BufferString errmsg;
    MouseCursorManager::setOverride( MouseCursor::Wait );
    const bool isok = dps.dataSet().putTo( ioobj.fullUserExpr(false), errmsg,
					   false);
    MouseCursorManager::restoreOverride();
    if ( !isok )
	mErrRet(mToUiStringTodo(errmsg))

    return true;
}
