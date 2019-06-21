/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2004
________________________________________________________________________

-*/

#include "uiseisioobjinfo.h"
#include "ioobj.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "uimsg.h"


#define mChk(ret) if ( !isOK() ) return ret

bool uiSeisIOObjInfo::provideUserInfo() const
{
    mChk(false);
    if ( is2D() )
	return provideLineInfo();

    uiPhraseSet inf;
    getUserInfo( inf );
    gUiMsg(parent_).info( inf );

    return true;
}


bool uiSeisIOObjInfo::provideLineInfo( const GeomIDSet* sel ) const
{
    mChk(false);

    GeomIDSet geomids;
    if ( sel )
	geomids = *sel;
    else
	getGeomIDs( geomids );

    const int nrlines = geomids.size();
    if ( nrlines < 1 )
	return false;

    uiPhraseSet inf;
    const BufferString datanm( ioObj()->name() );
    inf.add( nrlines < 2 ?
	    tr("The following line was added to dataset %1").arg(datanm)
	  : tr("%1 lines were added to dataset %2").arg(nrlines).arg(datanm) );

    const uiString zunitstr( SI().zUnitString() );
    const float zfac = SI().showZ2UserFactor();
    for ( int idx=0; idx<nrlines; idx++ )
    {
	const BufferString linenm = geomids[idx].name();
	StepInterval<int> trcrg( 0, 0, 1 );
	StepInterval<float> zrg( 0.f, 0.f, 1.f );
	if ( getRanges(geomids[idx],trcrg,zrg) )
	    zrg.scale( zfac );

	inf.add( tr("Line: %1, Trace range: %2 - %3 (step %4), "
		    "Z range (%5): %6 - %7 (step %8)")
		.arg(linenm).arg(trcrg.start).arg(trcrg.stop).arg(trcrg.step)
		.arg(zunitstr).arg(zrg.start).arg(zrg.stop).arg(zrg.step) );
    }

    gUiMsg(parent_).info( inf );
    return true;
}


bool uiSeisIOObjInfo::checkSpaceLeft( const SeisIOObjInfo::SpaceInfo& si,
				      bool doerrs ) const
{
    mChk(false);

    const int szmb = expectedMBs( si );
    if ( szmb < 0 ) // Unknown, but probably small
	return true;

    const int avszmb = System::getFreeMBOnDisk( *ioObj() );
#ifdef __win__
    const int szgb = szmb / 1024;
    if ( szgb >= 4 )
    {
	BufferString fsysname = System::getFileSystemName( ioObj()->dirName() );
	if ( fsysname == "FAT32" )
	{
	    gUiMsg(parent_).error(
		    tr("Target directory has a FAT32 File System."
		       "\nFiles larger than 4GB are not supported") );
	    return false;
	}
    }
#endif

    if ( avszmb == 0 )
    {
	if ( !doerrs ) return false;
	if ( !gUiMsg(parent_).askContinue( tr("The output disk is full.\n\n"
					      "Do you want to continue?") ) )
	    return false;
    }
    else if ( szmb > avszmb )
    {
	if ( !doerrs ) return false;
	uiString msg = tr("The new cube size may exceed the space "
		       "available on disk:\n%1\n\nDo you want to continue?");

	uiString explanationmsg = avszmb == 0 ? tr("The disk seems to be full!")
					      : tr("\nEstimated size: %1 MB\n"
						   "Available on disk: %2 MB")
					      .arg(szmb).arg(avszmb);

	msg.arg(explanationmsg);

	if ( !gUiMsg(parent_).askContinue( msg ) )
	    return false;
    }
    return true;
}
