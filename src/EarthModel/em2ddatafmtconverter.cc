/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ascstream.h"
#include "emioobjinfo.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "survgeom.h"


class OD_2DEMDataConverter_FromOD4ToOD5
{
public:
			    OD_2DEMDataConverter_FromOD4ToOD5()
			    {
				surfacepara_ = 0;
			    }
			    ~OD_2DEMDataConverter_FromOD4ToOD5()   {}

    void		    convertData(EM::ObjectType);

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
    converter.convertData( EM::ObjectType::Hor2D );
    converter.convertData( EM::ObjectType::FltSS2D3D );
}


void OD_2DEMDataConverter_FromOD4ToOD5::convertData( EM::ObjectType ftype )
{
    TypeSet<MultiID> ioobjids;
    EM::IOObjInfo::getIDs( ftype, ioobjids );
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( ioobjids[idx] );
	if ( !ioobj )
	    continue;

	EM::IOObjInfo ioobjinfo( ioobj.ptr() );
	if ( ioobjinfo.hasGeomIDs() )
	    continue;

	surfacepara_ = ioobjinfo.getPars();
	if ( !surfacepara_ )
	    continue;

	surfacepara_->set( sKey::GeomID(), "");
	if ( ftype == EM::ObjectType::Hor2D )
	    addGeomIDTo2DHorPara( ioobjinfo );
	else if ( isFaultStickSet(ftype) )
	    addGeomIDToFSSPara( ioobjinfo );

        if ( ioobjinfo.getParsOffsetInFile() > 0 )
	    writeToFile( ioobj->fullUserExpr(),	
					ioobjinfo.getParsOffsetInFile());
    }
}


void OD_2DEMDataConverter_FromOD4ToOD5::addGeomIDTo2DHorPara(
						     EM::IOObjInfo& ioobjinfo)
{
    TypeSet<Pos::GeomID> geomids;
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
	    mDefStickKey("Picked name",linekey,secids[secididx].asInt(),
			 stickidx);
	    surfacepara_->get( linekey.buf(), linename );
	    mDefStickKey("Picked MultiID",lsetkey,secids[secididx].asInt(),
			 stickidx);
	    MultiID lsid;
	    surfacepara_->get( lsetkey.buf(), lsid );
	    IOObj* lsioobj = IOM().get(lsid);
	    if ( !lsioobj ) continue;

	    Pos::GeomID geomid = Survey::GM().getGeomID( lsioobj->name(),
						 linename.buf() );
	    mDefStickKey("GeomID",geomstr,secids[secididx].asInt(),stickidx);
	    surfacepara_->set( geomstr, geomid );
	}
    }
}


void OD_2DEMDataConverter_FromOD4ToOD5::writeToFile( const char* fullpath,
						     int pos )
{
    FilePath fp( fullpath );
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
