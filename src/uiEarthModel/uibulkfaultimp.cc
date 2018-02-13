/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
________________________________________________________________________

-*/


#include "uibulkfaultimp.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"


class BulkFaultAscIO : public Table::AscIO
{
public:
BulkFaultAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
    , finishedreadingheader_(false)
{}


static Table::FormatDesc* getDesc( bool isfss, bool is2d )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkFault" );

    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    BufferString typnm = isfss ? "FaultStickSet name" : "Fault name";
    fd->bodyinfos_ += new Table::TargetInfo( typnm , Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    fd->bodyinfos_ += new Table::TargetInfo( "Stick number", IntInpSpec(),
					     Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Node index", IntInpSpec(),
					     Table::Optional );
/* Enable when supported
    if ( is2d )
	fd->bodyinfos_ += new Table::TargetInfo( "Line name", StringInpSpec(),
						 Table::Required );
*/

    return fd;
}


bool isXY() const
{ return formOf( false, 1 ) == 0; }


bool getData( BufferString& fltnm, Coord3& crd, int& sticknr )
{
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return false;

	udfval_ = getFValue( 0 );
	finishedreadingheader_ = true;
    }


    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    fltnm = text( 0 );
    if ( isXY() )
    {
	crd.x = getDValue( 1, udfval_ );
	crd.y = getDValue( 2, udfval_ );
	crd.z = getFValue( 3, udfval_ );
    }
    else
    {
	BinID bid( (int)getDValue(1,udfval_), (int)getDValue(2,udfval_) );
	crd = Coord3( SI().transform(bid), getFValue(3,udfval_) );
    }

    sticknr = getIntValue( 4 );
    return true;
}

    od_istream&		strm_;
    float		udfval_;
    bool		finishedreadingheader_;
};


#define mGet( tp, fss, f3d ) \
    FixedString(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : f3d

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D) )

#define mGetHelpKey(tp) \
    mGet( tp, (is2d ? mODHelpKey(mImportFaultStick2DHelpID) \
		    : mODHelpKey(mImportFaultStick3DHelpID) ), \
    mODHelpKey(mImportFaultHelpID) )


uiBulkFaultImport::uiBulkFaultImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Multiple Faults"),mNoDlgTitle,
				 mODHelpKey(mBulkFaultImportHelpID))
		 .modal(false))
    , isfss_(false)
    , fd_(BulkFaultAscIO::getDesc(false,false))
{
    init();
}


uiBulkFaultImport::uiBulkFaultImport( uiParent* p, const char* type, bool is2d )
    : uiDialog(p,uiDialog::Setup(mGet( type, (is2d
			      ? tr("Import Multiple FaultStickSets 2D")
			      : tr("Import Multiple FaultStickSets")),
				tr("Import Multiple Faults") ),mNoDlgTitle,
				mGetHelpKey(type)).modal(false))
    , isfss_(mGet(type,true,false))
    , fd_(BulkFaultAscIO::getDesc(isfss_,is2d))
{
    init();
}


void uiBulkFaultImport::init()
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiFileInput( this,
		      uiStrings::sInputASCIIFile(),
		      uiFileInput::Setup().withexamine(true)
		      .examstyle(File::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
		mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkFaultImport::~uiBulkFaultImport()
{
    delete fd_;
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiBulkFaultImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("the input file name")) )
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpen(uiStrings::sInputFile().toLower()))

    if ( !dataselfld_->commit() )
	return false;

    BufferStringSet fltnms;
    TypeSet< TypeSet<Coord3> > nodes;
    TypeSet< TypeSet<int> > sticks;

    BulkFaultAscIO aio( *fd_, strm );
    BufferString fltnm; Coord3 crd; int sticknr;
    while ( aio.getData(fltnm,crd,sticknr) )
    {
	if ( fltnm.isEmpty() || !crd.isDefined() || mIsUdf(sticknr)  )
	    continue;

	const int fltidx = fltnms.indexOf( fltnm );
	if ( fltidx==-1 )
	{
	    fltnms.add( fltnm );
	    TypeSet<Coord3> poses; poses += crd; nodes += poses;
	    TypeSet<int> stickids; stickids += sticknr; sticks += stickids;
	}
	else
	{
	    nodes[fltidx] += crd;
	    sticks[fltidx] += sticknr;
	}
    }

    // TODO: Check if name exists, ask user to overwrite or give new name
    BufferStringSet errors;
    uiTaskRunner taskr( this );
    const Coord3 normal( 0, 0, 1 );
    const char* typestr = isfss_ ? EM::FaultStickSet::typeStr()
				 : EM::Fault3D::typeStr();
    for ( int idx=0; idx<fltnms.size(); idx++ )
    {
	EM::ObjectID eid = EM::EMM().createObject( typestr, fltnms.get(idx) );
	EM::EMObject* emobj = EM::EMM().getObject( eid );
	emobj->ref();
	mDynamicCastGet( EM::Fault3D*, emflt, emobj );
	mDynamicCastGet( EM::FaultStickSet*, emfss, emobj );

	if ( !emflt && !emfss )
	{
	    emobj->unRef();
	    continue;
	}

	Geometry::FaultStickSet& flt = emflt ?
				    *emflt->geometry().sectionGeometry(0) :
				    *emfss->geometry().sectionGeometry(0);
	TypeSet<int> usedstickids;

	for ( int sidx=0; sidx<sticks[idx].size(); sidx++ )
	{
	    const int cursidx = usedstickids.indexOf( sticks[idx][sidx] );
	    if ( cursidx < 0 )
	    {
		const int curstickidx = usedstickids.size();
		usedstickids += sticks[idx][sidx];
		flt.insertStick( nodes[idx][sidx], normal, curstickidx );
	    }
	    else
	    {
		const TypeSet<Coord3>* poses = flt.getStick( cursidx );
		const int lastidx = poses ? poses->size() : 0;
		flt.insertKnot( RowCol(cursidx,lastidx), nodes[idx][sidx] );
	    }
	}

	PtrMan<Executor> saver =  emflt ? emflt->saver() :  emfss->saver();
	if ( !saver || !TaskRunner::execute( &taskr, *saver )  )
	{
	    uiMSG().error(tr("Cannot import %1").arg(toUiString(typestr)));
	    return false;
	}
	emobj->unRef();
    }

    uiMSG().message(tr("Imported all %1 from file %2").arg(toUiString(typestr))
							.arg(fnm));
    return true;
}
