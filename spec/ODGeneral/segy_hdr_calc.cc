/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2006
-*/

static const char* rcsID = "$Id";

#include "strmprov.h"
#include "ibmformat.h"
#include "progressmeter.h"

#define mErrRet(s) { std::cerr << argv[0] << ": " << s << std::endl; return 1; }

int main( int argc, char** argv )
{
    if ( argc < 3 )
	mErrRet("Two arguments required: input_segy_file output_segy_file")

    StreamData sd = StreamProvider(argv[1]).makeIStream();
    if ( !sd.usable() )
	mErrRet("Cannot open input file")
    std::istream& instrm = *sd.istrm;

    sd = StreamProvider(argv[2]).makeOStream();
    if ( !sd.usable() )
	mErrRet("Cannot open output file")
    std::ostream& outstrm = *sd.ostrm;

    char buf[100000];
    instrm.read( buf, 3600 );
    outstrm.write( buf, 3600 );
    const int ns = IbmFormat::asShort( buf + 3220 );
    const int fmt = IbmFormat::asShort( buf + 3224 );
    const int nrdatabytes = ns * (fmt == 3 ? 2 : (fmt == 8 ? 1 : 4));
    std::cerr << "ns=" << ns << " fmt=" << fmt << " nrdatabytes="
		<< nrdatabytes << std::endl;
    const int trcbytes = 240 + nrdatabytes;

    ProgressMeter pm( std::cerr );
    while ( true )
    {
	instrm.read( buf, trcbytes );
	if ( instrm.bad() || instrm.eof() ) break;
	++pm;

	const int inl = IbmFormat::asInt( buf + 4 );
	const int crl = IbmFormat::asInt( buf + 20 );
	const double fcdp = (inl - 170) * 1126.5 + crl - 378;
	const int cdp = mNINT32(fcdp);
	IbmFormat::putInt( cdp, buf );
	outstrm.write( buf, trcbytes );
	if ( !outstrm.good() )
	    mErrRet("Cannot write trace")
    }

    pm.finish();
    sd.close();

    return 0;
}
