/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisioobjinfo.h"

#include "cbvsreadmgr.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "od_strstream.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "survinfo.h"
#include "systeminfo.h"

#include "uimsg.h"


uiSeisIOObjInfo::uiSeisIOObjInfo( const IOObj& ioobj, bool errs )
    : sii_(ioobj)
    , doerrs_(errs)
{
}


uiSeisIOObjInfo::uiSeisIOObjInfo( const MultiID& key, bool errs )
    : sii_(key)
    , doerrs_(errs)
{
}


uiSeisIOObjInfo::~uiSeisIOObjInfo()
{}


#define mChk(ret) if ( !isOK() ) return ret

bool uiSeisIOObjInfo::provideUserInfo() const
{
    mChk(false);
    if ( is2D() )
	return provideUserInfo2D();

    PtrMan<Translator> t = ioObj()->createTranslator();
    if ( !t )
	{ pErrMsg("No Translator"); return true; }
    mDynamicCastGet(CBVSSeisTrcTranslator*,trans,t.ptr());
    if ( !trans )
	{ return true; }

    Conn* conn = ioObj()->getConn( Conn::Read );
    if ( !conn || !trans->initRead(conn) )
    {
	if ( doerrs_ )
	    uiMSG().error( tr("No output cube produced") );
	return false;
    }

    od_ostrstream strm;
    strm << "The cube is available for work.\n\n";
    trans->readMgr()->dumpInfo( strm, false );
    uiMSG().message( mToUiStringTodo(strm.result()) );

    return true;
}


bool uiSeisIOObjInfo::provideUserInfo2D( const TypeSet<Pos::GeomID>* sel ) const
{
    mChk(false);
    //TODO: Handle errors

    TypeSet<Pos::GeomID> geomids;
    if ( sel )
	geomids = *sel;
    else
	sii_.getGeomIDs( geomids );

    const int nrlines = geomids.size();
    const BufferString datanm( sii_.ioObj()->name() );
    uiString msg = nrlines < 2 ?
	tr("The following line was added to dataset %2:\n").arg(datanm)
	: tr("%1 lines were added to dataset %2:\n").arg(nrlines).arg(datanm);
    const uiString zunitstr( SI().getUiZUnitString() );
    const float zfac = SI().showZ2UserFactor();
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const BufferString linenm = Survey::GM().getName( geomids[idx] );
	StepInterval<int> trcrg( 0, 0, 1 );
	StepInterval<float> zrg( 0.f, 0.f, 1.f );
	if ( sii_.getRanges(geomids[idx],trcrg,zrg) )
	    zrg.scale( zfac );

	msg.append( tr("Line: %1, Trace range: %2 to %3 (step %4), "
		    "Z range %5: %6 to %7 (step %8)\n")
                    .arg(linenm).arg(trcrg.start_).arg(trcrg.stop_).arg(trcrg.step_)
                    .arg(zunitstr).arg(zrg.start_).arg(zrg.stop_).arg(zrg.step_) );
    }

    uiMSG().message( msg);
    return true;
}


int uiSeisIOObjInfo::expectedMBs( const SeisIOObjInfo::SpaceInfo& si ) const
{
    const od_int64 szbytes = si.expectedSize();
    return szbytes / mDef1MB;
}


bool uiSeisIOObjInfo::checkSpaceLeft( const SeisIOObjInfo::SpaceInfo& si ) const
{
    mChk(false);

    const od_int64 szbyte = expectedSize( si );
    if ( szbyte < 0 ) // Unknown, but probably small
	return true;

    const double szmb = szbyte / mDef1MB;
    if ( __iswin__ )
    {
	const BufferString fsysname =
		System::getFileSystemName( ioObj()->dirName() );
	const double szgb = szmb / mDef1KB;
	if ( fsysname == "FAT32" && szgb>=4.0 )
	{
	    uiMSG().error( tr("Target folder has a FAT32 File System.\n"
			      "Files larger than 4GB are not supported") );
	    return false;
	}
    }

    const double avszmb = System::getFreeMBOnDisk( *ioObj() );
    if ( avszmb == 0 )
    {
	if ( !doerrs_ )
	    return false;

	if ( !uiMSG().askContinue( tr("The output disk seems to be full.\n\n"
				      "Do you want to continue?") ) )
	    return false;
    }
    else if ( szmb > avszmb )
    {
	if ( !doerrs_ )
	    return false;

	uiString msg = tr( "The new cube size may exceed the space "
		       "available on disk:\n%1\n\nDo you want to continue?" );
	uiString explanationmsg = avszmb == 0 ? tr("The disk seems to be full!")
					      : tr("\nEstimated size: %1 MB\n"
						   "Available on disk: %2 MB")
					      .arg(szmb).arg(avszmb);
	msg.arg( explanationmsg );
	if ( !uiMSG().askContinue(msg) )
	    return false;
    }

    return true;
}
