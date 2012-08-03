/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2010
-*/

static const char* rcsID mUnusedVar = "$Id: seisgeometryfrom2dlines.cc,v 1.4 2012-08-03 13:01:35 cvskris Exp $";

#include "bufstringset.h"
#include "file.h"
#include "ioobj.h"
#include "ioman.h"
#include "mousecursor.h"
#include "seis2dline.h"
#include "seisioobjinfo.h"
#include "surv2dgeom.h"
#include "survinfo.h"


class OD_2DLineGeometryFrom2DLinesTransf : public CallBacker
{
public:

    void	doTransf(CallBacker* cb=0);

};


mGlobal(Seis) void OD_Init_Transf_2DLineGeometry_From_2D_SeisLines(); // compiler
mGlobal(Seis) void OD_Init_Transf_2DLineGeometry_From_2D_SeisLines()
{
    static OD_2DLineGeometryFrom2DLinesTransf transf;
    transf.doTransf();
    IOM().surveyChanged.notify(
	    mCB(&transf,OD_2DLineGeometryFrom2DLinesTransf,doTransf) );
}


void OD_2DLineGeometryFrom2DLinesTransf::doTransf( CallBacker* )
{
    if ( !SI().has2D() ) return;

    BufferStringSet lsnms; TypeSet<MultiID> lsids;
    SeisIOObjInfo::get2DLineInfo( lsnms, &lsids );
    if ( lsnms.isEmpty() ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );

    for ( int ils=0; ils<lsids.size(); ils++ )
    {
	IOObj* ioobj = IOM().get( lsids[ils] );
	if ( !ioobj ) continue;
	Seis2DLineSet ls( *ioobj );
	delete ioobj;
	if ( ls.nrLines() < 1 ) continue;

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
		PosInfo::GeomID geomid =
		    PosInfo::POS2DAdmin().getGeomID( ls.name(), lnm );
		if ( !geomid.isOK() )
		    PosInfo::POS2DAdmin().setGeometry( l2dd );
		linesdone.add( lnm );
	    }
	}
    }

    MouseCursorManager::restoreOverride();
}
