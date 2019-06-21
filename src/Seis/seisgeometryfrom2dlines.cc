/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2010
-*/

#include "bufstringset.h"
#include "file.h"
#include "ioobj.h"
#include "dbman.h"
#include "mousecursor.h"
#include "seis2dline.h"
#include "seisioobjinfo.h"
#include "posinfo2dsurv.h"
#include "survinfo.h"


class OD_2DLineGeometryFrom2DLinesTransf : public CallBacker
{
public:

    void	doTransf(CallBacker* cb=0);

};


mGlobal(Seis) void OD_Init_Transf_2DLineGeometry_From_2D_SeisLines(); // compiler
mGlobal(Seis) void OD_Init_Transf_2DLineGeometry_From_2D_SeisLines()
{
    mDefineStaticLocalObject( OD_2DLineGeometryFrom2DLinesTransf, transf, );
    transf.doTransf();
    DBM().surveyChanged.notify(
	    mCB(&transf,OD_2DLineGeometryFrom2DLinesTransf,doTransf) );
}


void OD_2DLineGeometryFrom2DLinesTransf::doTransf( CallBacker* )
{
    if ( !SI().has2D() )
	return;

    BufferStringSet lsnms; DBKeySet lsids;
    SeisIOObjInfo::get2DLineInfo( lsnms, &lsids );
    if ( lsnms.isEmpty() )
	return;

    MouseCursorManager::setOverride( MouseCursor::Wait );

    for ( int ils=0; ils<lsids.size(); ils++ )
    {
	IOObj* ioobj = lsids[ils].getIOObj();
	if ( !ioobj )
	    continue;
	Seis2DLineSet ls( *ioobj );
	delete ioobj;
	if ( ls.nrLines() < 1 )
	    continue;

	S2DPOS().setCurLineSet( ls.name() );
	BufferStringSet linesdone;
	for ( int iln=0; iln<ls.nrLines(); iln++ )
	{
	    const BufferString lnm( ls.lineName(iln) );
	    if ( linesdone.isPresent(lnm) )
		continue;

	    PosInfo::Line2DData l2dd;
	    const bool doupdate = !S2DPOS().hasLine( lnm.buf() );
	    if ( doupdate && ls.getGeometry(iln,l2dd) )
	    {
		l2dd.setLineName( lnm );
		PosInfo::Line2DKey l2dkey =
		    PosInfo::POS2DAdmin().getLine2DKey( ls.name(), lnm );
		if ( !l2dkey.isOK() )
		    PosInfo::POS2DAdmin().setGeometry( l2dd );
		linesdone.add( lnm );
	    }
	}
    }

    MouseCursorManager::restoreOverride();
}
