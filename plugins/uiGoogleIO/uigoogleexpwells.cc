/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/

static const char* rcsID = "$Id";

#include "uigoogleexpwells.h"
#include "odgooglexmlwriter.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "strmprov.h"
#include "survinfo.h"
#include "welltransl.h"
#include "welldata.h"
#include "wellreader.h"
#include "iodirentry.h"
#include "ioman.h"
#include "latlong.h"
#include <iostream>


uiGoogleExportWells::uiGoogleExportWells( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Wells to KML",
				 "Specify wells to output","0.3.10") )
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Wells", true );
    selfld_ = llb->box();

    fnmfld_ = new uiFileInput( this, "Output file",
		uiFileInput::Setup(uiFileDialog::Gen,GetBaseDataDir())
		.forread(false).filter("*.kml;;*.kmz") );
    fnmfld_->attach( alignedBelow, llb );

    finaliseStart.notify( mCB(this,uiGoogleExportWells,initWin) );
}


uiGoogleExportWells::~uiGoogleExportWells()
{
}


void uiGoogleExportWells::initWin( CallBacker* )
{
    IOM().to( WellTranslatorGroup::ioContext().getSelKey() );
    IODirEntryList del( IOM().dirPtr(), WellTranslatorGroup::ioContext() );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	IODirEntry* de = del[idx];
	if ( de && de->ioobj )
	{
	    selfld_->addItem( de->name() );
	    wellids_ += new MultiID( de->ioobj->key() );
	}
    }
}



#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGoogleExportWells::acceptOK( CallBacker* )
{
    const BufferString fnm( fnmfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet("Please enter a file name" )

    ODGoogle::XMLWriter wrr( "Wells", fnm, SI().name() );
    if ( !wrr.isOK() )
	mErrRet(wrr.errMsg())

    wrr.strm() << "\t<Style id=\"sn_od_wellpin0\">\n"
	"\t\t<IconStyle>\n"
	"\t\t\t<scale>1.1</scale>\n"
	"\t\t\t<Icon>\n"
	"\t\t\t\t<href>http://opendtect.org/images/od-wellpin.png</href>\n"
	"\t\t\t</Icon>\n"
	"\t\t<hotSpot x=\"20\" y=\"2\" xunits=\"pixels\" yunits=\"pixels\"/>\n"
	"\t\t</IconStyle>\n"
	"\t</Style>\n\n";

    wrr.strm() << "\t<StyleMap id=\"msn_od_wellpin\">\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>normal</key>\n"
	"\t\t\t<styleUrl>#sn_od_wellpin0</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t\t<Pair>\n"
	"\t\t\t<key>highlight</key>\n"
	"\t\t\t<styleUrl>#od-wellpin</styleUrl>\n"
	"\t\t</Pair>\n"
	"\t</StyleMap>\n\n";

    wrr.strm() << 
	"\t<Style id=\"od-wellpin\">\n"
	"\t\t<IconStyle>\n"
	"\t\t\t<scale>1.3</scale>\n"
	"\t\t\t<Icon>\n"
	"\t\t\t\t<href>http://opendtect.org/images/od-wellpin.png</href>\n"
	"\t\t\t</Icon>\n"
	"\t\t\t<hotSpot x=\"20\" y=\"2\" xunits=\"pixels\" "
					"yunits=\"pixels\"/>\n"
	"\t\t</IconStyle>\n"
	"\t</Style>\n\n" << std::endl;

    Well::Data wd;
    for ( int idx=0; idx<selfld_->size(); idx++ )
    {
	if ( !selfld_->isSelected(idx) )
	    continue;

	Well::Reader wllrdr( Well::IO::getMainFileName( *wellids_[idx] ), wd );
	if ( !wllrdr.getInfo() )
	    continue;

	if ( !writeWell(wrr.strm(),selfld_->textOfItem(idx),wd.info()) )
	    { wrr.close(); mErrRet("Error during write"); }
    }

    wrr.close();
    return true;
}


bool uiGoogleExportWells::writeWell( std::ostream& strm, const char* nm,
					const Well::Info& wi )
{
    const LatLong ll( SI().latlong2Coord().transform(wi.surfacecoord) );

    strm << "\n\t<Placemark>\n"
	<< "\t\t<name>" << nm << "</name>\n"
	<< "\t\t<LookAt>\n";
    strm << "\t\t\t<longitude>" << getStringFromDouble(0,ll.lng_)
				      << "</longitude>\n";
    strm << "\t\t\t<latitude>" << getStringFromDouble(0,ll.lat_)
				      << "</latitude>\n";
    strm << "\t\t\t<altitude>0</altitude>\n"
	"\t\t\t<range>500</range>\n"
	"\t\t\t<tilt>20</tilt>\n"
	"\t\t\t<heading>0</heading>\n"
	"\t\t\t<altitudeMode>relativeToGround</altitudeMode>\n"
	"\t\t</LookAt>\n"
	"\t\t<styleUrl>#msn_od_wellpin</styleUrl>\n"
	"\t\t<Point>\n"
	"\t\t\t<coordinates>" << getStringFromDouble(0,ll.lng_);
    strm << ',' << getStringFromDouble(0,ll.lat_) << ",0</coordinates>\n"
	"\t\t</Point>\n"
	"\t</Placemark>" << std::endl;

    return strm.good();
}
