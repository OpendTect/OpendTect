/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Jan 2014
-*/



#include "ascstream.h"
#include "emioobjinfo.h"
#include "emhorizon2d.h"
#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "safefileio.h"
#include "survgeom.h"


class OD_2DEMDataConverter_FromOD4ToOD5
{
public:
			    OD_2DEMDataConverter_FromOD4ToOD5()
			    {
				surfacepara_ = 0;
			    }
			    ~OD_2DEMDataConverter_FromOD4ToOD5()   {}

    void		    convertData(EM::IOObjInfo::ObjectType);

protected:

    void		    addGeomIDTo2DHorPara(EM::IOObjInfo&);
    void		    addGeomIDToFSSPara(EM::IOObjInfo&);
    void		    writeToFile(const char*,int);

    IOPar*		    surfacepara_;
};


mGlobal(EarthModel) void OD_Convert_EM2DData();
mGlobal(EarthModel) void OD_Convert_EM2DData()
{
    mDefineStaticLocalObject( OD_2DEMDataConverter_FromOD4ToOD5, converter, );
    converter.convertData( EM::IOObjInfo::Horizon2D );
    converter.convertData( EM::IOObjInfo::FaultStickSet );
}


void OD_2DEMDataConverter_FromOD4ToOD5::convertData(
                                            EM::IOObjInfo::ObjectType ftype )
{
    DBKeySet ioobjids;
    EM::IOObjInfo::getIDs( ftype, ioobjids );
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = ioobjids[idx].getIOObj();
	if ( !ioobj )
	    continue;
	EM::IOObjInfo ioobjinfo( ioobj );
	if ( ioobjinfo.hasGeomIDs() ) continue;
	surfacepara_ = ioobjinfo.getPars();
	if ( !surfacepara_ )
	    continue;

	surfacepara_->set( sKey::GeomID(), "");
	if ( ftype == EM::IOObjInfo::Horizon2D )
	    addGeomIDTo2DHorPara( ioobjinfo );
	else if ( ftype == EM::IOObjInfo::FaultStickSet )
	    addGeomIDToFSSPara( ioobjinfo );

        if ( ioobjinfo.getParsOffsetInFile() > 0 )
	    writeToFile(ioobj->mainFileName(), ioobjinfo.getParsOffsetInFile());
    }
}


void OD_2DEMDataConverter_FromOD4ToOD5::addGeomIDTo2DHorPara(
                                                       EM::IOObjInfo& ioobjinfo)
{
    GeomIDSet geomids;
    TypeSet< StepInterval<int> > trcranges;
    ioobjinfo.getGeomIDs( geomids );
    for ( int idy=0; idy<geomids.size(); idy++ )
	surfacepara_->set( IOPar::compKey(sKey::GeomID(),idy), geomids[idy] );
}


#define mDefStickKey( prefixstr, strname, sid, sticknr ) \
    BufferString strname(prefixstr); strname += " of section "; \
    strname += sid; strname += " sticknr "; strname += sticknr;


void OD_2DEMDataConverter_FromOD4ToOD5::addGeomIDToFSSPara(
                                                    EM::IOObjInfo& ioobjinfo )
{
    TypeSet<EM::SectionID> secids;
    ioobjinfo.getSectionIDs( secids );
    int nrsticks = ioobjinfo.nrSticks();

    BufferString linename;
    for ( int secididx=0; secididx<secids.size(); secididx++ )
    {
	for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
	{
	    mDefStickKey("Picked name",linekey,secids[secididx],stickidx);
	    surfacepara_->get( linekey.buf(), linename );
	    mDefStickKey("Picked DBKey",lsetkey,secids[secididx],stickidx);
	    DBKey lsid;
	    surfacepara_->get( lsetkey.buf(), lsid );
	    PtrMan<IOObj> lsioobj = lsid.getIOObj();
	    if ( !lsioobj )
		continue;

	    BufferString oldlnm( lsioobj->name().str(), "-", linename );
	    auto geomid = SurvGeom::getGeomID( oldlnm );
	    mDefStickKey("GeomID",geomstr,secids[secididx],stickidx);
	    surfacepara_->set( geomstr, geomid );
	}
    }
}


void OD_2DEMDataConverter_FromOD4ToOD5::writeToFile( const char* fullpath,
                                                     int pos )
{
    File::Path fp( fullpath );
    od_ostream ostrm( fp, true );
    if ( !ostrm.isOK() )
        return;

    ostrm.setWritePosition( pos );
    ascostream parstream(ostrm);
    parstream.stream() << od_endl;
    parstream.newParagraph();
    surfacepara_->putTo( parstream );
    ostrm.close();
    return;
}
