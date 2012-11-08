/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 1995
 * FUNCTION : Seg-Y headers
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "segythdef.h"
#include "ibmformat.h"
#include "settings.h"
#include <string.h>

static const char* sKeyBytesFor = "Nr bytes for ";


namespace SEGY
{
const char* TrcHeaderDef::sInlByte()	{ return "In-line byte"; }
const char* TrcHeaderDef::sCrlByte()	{ return "Cross-line byte"; }
const char* TrcHeaderDef::sXCoordByte()	{ return "X-coord byte"; }
const char* TrcHeaderDef::sYCoordByte()	{ return "Y-coord byte"; }
const char* TrcHeaderDef::sOffsByte()	{ return "Offset byte"; }
const char* TrcHeaderDef::sAzimByte()	{ return "Azimuth byte"; }
const char* TrcHeaderDef::sTrNrByte()	{ return "Trace number byte"; }
const char* TrcHeaderDef::sRefNrByte()	{ return "Reference number byte"; }
const char* TrcHeaderDef::sPickByte()	{ return "Pick byte"; }
}


SEGY::HdrEntry& SEGY::HdrEntry::operator =( const SEGY::HdrEntry& he )
{
    if ( this != &he )
    {
	bytepos_ = he.bytepos_;
	small_ = he.small_;
	type_ = he.type_;
	setDescription( he.desc_ );
	setName( he.name_ );
    }
    return *this;
}


const char* SEGY::HdrEntry::description() const
{
    if ( desc_ )
	return desc_;
    else if ( isUdf() )
	return "";

    static BufferString ret( "Byte ", (int)bytepos_, "(" );
    ret.add( byteSize() ).add( " bytes, type " );
    if ( type_ == UInt )
	ret.add( "un" );
    ret.add( type_ == Float ? "float" : "signed int" ).add( ")" );
    return ret.buf();
}


const char* SEGY::HdrEntry::name() const
{
    return name_ ? name_ : "";
}


void SEGY::HdrEntry::setName( const char* nm )
{
    delete name_;
    if ( !nm )
	name_ = 0;
    else
	{ name_ = new char [strlen(nm)+1]; strcpy(name_,nm); }
}


void SEGY::HdrEntry::setDescription( const char* d )
{
    delete desc_;
    if ( !d )
	desc_ = 0;
    else
	{ desc_ = new char [strlen(d)+1]; strcpy(desc_,d); }
}


static const unsigned char* getBytes( const void* inpbuf, bool swapped,
				      int bytenr, int nrbytes )
{
    static unsigned char swpbuf[4];
    const unsigned char* ptr = ((const unsigned char*)inpbuf) + bytenr;
    if ( !swapped ) return ptr;

    memcpy( swpbuf, ptr, nrbytes );
    SwapBytes( swpbuf, nrbytes );
    return swpbuf;
}


#define mGetBytes() getBytes(buf,swapped,bytepos_,byteSize())

int SEGY::HdrEntry::getValue( const void* buf, bool swapped ) const
{
    if ( bytepos_ < 0 )
	return 0;

    if ( small_ )
	return type_ == UInt ? IbmFormat::asUnsignedShort( mGetBytes() )
	    		     : IbmFormat::asShort( mGetBytes() );
    else if ( type_ == UInt )
	return IbmFormat::asUnsignedShort( mGetBytes() );
    else if ( type_ == Float )
	return (int)( *( (float*)mGetBytes() ) );
    		// If they are stupid enough to put floats in SEG-Y headers,
    		// then they will probably not do that in IbmFormat

    return IbmFormat::asInt( mGetBytes() );
}


void SEGY::HdrEntry::putValue( void* buf, int val ) const
{
    if ( bytepos_ < 0 )
	return;

    unsigned char* ptr = ((unsigned char*)buf) + bytepos_;
    if ( small_ )
    {
	if ( type_ == UInt )
	    IbmFormat::putUnsignedShort( (unsigned short)val, ptr );
	else
	    IbmFormat::putShort( (short)val, ptr );
    }
    else if ( type_ == UInt )
	IbmFormat::putUnsignedShort( (unsigned short)val, ptr );
    else if ( type_ == Float )
    {
	float fval = (float)val; // see remark in getValue()
	memcpy( ptr, &fval, sizeof(float) );
    }
    else
	IbmFormat::putInt( (int)val, ptr );
}


void SEGY::HdrEntry::fillPar( IOPar& iop, const char* ky ) const
{
    if ( isUdf() )
	removeFromPar( iop, ky );
    else
    {
	iop.set( ky, bytepos_ );
	iop.set( BufferString(sKeyBytesFor,ky), small_ ? 2 : 4 );
    }
}


void SEGY::HdrEntry::usePar( const IOPar& iop, const char* ky )
{
    int byt = bytepos_; iop.get( ky, byt );
    byt -= byt % 2; // to support old stuff storing this in 'user' byte numbers
    bytepos_ = (HdrEntry::BytePos)byt;
    int nb = small_ ? 2 : 4;
    iop.get( BufferString(sKeyBytesFor,ky), nb );
    small_ = nb < 4;
}


void SEGY::HdrEntry::removeFromPar( IOPar& iop, const char* ky ) const
{
    iop.removeWithKey( ky );
    iop.removeWithKey( BufferString(sKeyBytesFor,ky) );
}


#define mAddHdr(nm,issmll,desc) \
    he = new HdrEntry( 0, issmll, dtyp ); \
    he->setName( nm ); he->setDescription( desc ); \
    *this += he
#define mAddHead(nm,desc) mAddHdr(nm,true,desc)
#define mAddHead4(nm,desc) mAddHdr(nm,false,desc)


void SEGY::HdrDef::mkTrc()
{
    HdrEntry::DataType dtyp = HdrEntry::SInt;
    HdrEntry* he;
    mAddHead4( "tracl", "trace sequence number within line" );
    mAddHead4( "tracr", "trace sequence number within reel" );
    mAddHead4( "fldr", "field record number" );
    mAddHead4( "tracf", "trace number within field record" );
    mAddHead4( "ep", "energy source point number" );
    mAddHead4( "cdp", "CDP ensemble number" ); // 5
    mAddHead4( "cdpt", "trace number within CDP ensemble" );
    mAddHead( "trid", "trace identification code: anything < 2 is good." );
    mAddHead( "nvs", "number of vertically summed traces" );
    mAddHead( "nhs", "number of horizontally summed traces" );
    mAddHead( "duse", "data use: 1 = production 2 = test" ); // 10
    mAddHead4( "offset", "distance from source point to receiver group (negative if opposite to direction in which the line was shot)" );
    mAddHead4( "gelev", "receiver group elevation from sea level (above sea level is positive)" );
    mAddHead4( "selev", "source elevation from sea level (above sea level is positive)" );
    mAddHead4( "sdepth", "source depth (positive)" );
    mAddHead4( "gdel", "datum elevation at receiver group" ); // 15
    mAddHead4( "sdel", "datum elevation at source" );
    mAddHead4( "swdep", "water depth at source" );
    mAddHead4( "gwdep", "water depth at receiver group" );
    mAddHead( "scalel", "scale factor for gelev-gwdep fields with value plus or minus 10 to the power 0, 1, 2, 3, or 4 (if positive, multiply, if negative divide)" );
    mAddHead( "scalco", "scale factor for coordinate fields (sx, sy, gx and gy) with value plus or minus 10 to the power 0, 1, 2, 3, or 4 (if positive, multiply, if negative divide)" ); // 20
    mAddHead4( "sx", "X source coordinate" );
    mAddHead4( "sy", "Y source coordinate" );
    mAddHead4( "gx", "X group coordinate" );
    mAddHead4( "gy", "Y group coordinate" );
    mAddHead( "counit", "coordinate units code for sx, sy, gx and gy: 1 = length (meters or feet), 2 = seconds of arc, 3 = decimal degrees, 4 = degrees, minutes, seconds" ); // 25
    mAddHead( "wevel", "weathering velocity" );
    mAddHead( "swevel", "subweathering velocity" );
    mAddHead( "sut", "uphole time at source" );
    mAddHead( "gut", "uphole time at receiver group" );
    mAddHead( "sstat", "source static correction" ); // 30
    mAddHead( "gstat", "group static correction" );
    mAddHead( "tstat", "total static applied" );
    mAddHead( "laga", "lag time A, time in ms between end of 240-byte trace identification header and time break, positive if time break occurs after end of header" );
    mAddHead( "lagb", "lag time B, time in ms between the time break and the initiation time of the energy source, may be positive or negative" );
    mAddHead( "delrt", "delay recording time, time in ms between initiation time of energy source and time when recording of data samples begins" ); // 35
    mAddHead( "muts", "mute time--start" );
    mAddHead( "mute", "mute time--end" ); // 37

	/* This is NOT defined in the SEG-Y standard. But: SU does this and it
	makes sense, so I followed their lead. It extends the Z possibilities
	of SEG-Y with a factor 2.
	Shows how easy it is to not see what's coming: of course these should
	have been 4-byte fields. */
    dtyp = HdrEntry::UInt;
    mAddHead( "ns", "number of samples in this trace" ); // 38
    mAddHead( "dt", "sample interval; in micro-seconds" ); // 39

    dtyp = HdrEntry::SInt;
    mAddHead( "gain", "gain type of field instruments code" ); // 40
    mAddHead( "igc", "instrument gain constant" );
    mAddHead( "igi", "instrument early or initial gain" );
    mAddHead( "corr", "correlated: 1 = no 2 = yes" );
    mAddHead( "sfs", "sweep frequency at start" );
    mAddHead( "sfe", "sweep frequency at end" ); // 45
    mAddHead( "slen", "sweep length in ms" );
    mAddHead( "styp", "sweep type code" );
    mAddHead( "stas", "sweep trace length at start in ms" );
    mAddHead( "stae", "sweep trace length at end in ms" );
    mAddHead( "tatyp", "taper type: 1=linear, 2=cos^2, 3=other" ); // 50
    mAddHead( "afilf", "alias filter frequency if used" );
    mAddHead( "afils", "alias filter slope" );
    mAddHead( "nofilf", "notch filter frequency if used" );
    mAddHead( "nofils", "notch filter slope" );
    mAddHead( "lcf", "low cut frequency if used" );
    mAddHead( "hcf", "high cut frequncy if used" );
    mAddHead( "lcs", "low cut slope" );
    mAddHead( "hcs", "high cut slope" );
    mAddHead( "year", "year data recorded" );
    mAddHead( "day", "day of year" ); // 60
    mAddHead( "hour", "hour of day (24 hour clock)" );
    mAddHead( "minute", "minute of hour" );
    mAddHead( "sec", "second of minute" );
    mAddHead( "timbas", "time basis code" );
    mAddHead( "trwf", "trace weighting factor" );
    mAddHead( "grnors", "geophone group number of roll switch position one" );
    mAddHead( "grnofr", "geophone group number of trace one within original field record" );
    mAddHead( "grnlof", "geophone group number of last trace within original field record" );
    mAddHead( "gaps", "gap size (total number of groups dropped)" );
    mAddHead( "otrav", "overtravel taper code" ); // 70
    mAddHead4( "Xcdp", "X coordinate of CDP (scalco applies)" ); // 71
    mAddHead4( "Ycdp", "Y coordinate of CDP (scalco applies)" ); // 72
    mAddHead4( "Inline", "Inline number of CDP" ); // 73
    mAddHead4( "Crossline", "Crossline number of CDP" ); // 74
    mAddHead4( "SP", "Shotpoint number" ); // 75
    mAddHead( "SPscale", "Shotpoint number scaling (as scalco)" ); // 76
    mAddHead( "Tunit", "Trace value measurement unit" );
    mAddHead( "TCpart1", "Transduction Constant (part 1)" );
    mAddHead( "TCpart2", "Transduction Constant (part 2)" );
    mAddHead( "TCpart3", "Transduction Constant (part 3)" ); // 80
    mAddHead( "TU", "Transduction Unit" );
    mAddHead( "DTI", "Device/Trace Identifier" );
    mAddHead( "TimSc", "Time Scaler" );
    mAddHead( "STO", "Source Type/Orientation" );
    mAddHead( "SEDpart1", "Source Energy Direction part 1" );
    mAddHead( "SEDpart2", "Source Energy Direction part 2" );
    mAddHead( "SEDpart3", "Source Energy Direction part 3" );
    mAddHead( "SMpart1", "Source Measurement part 1" );
    mAddHead( "SMpart2", "Source Measurement part 2" );
    mAddHead( "SMpart3", "Source Measurement part 3" ); // 90
    mAddHead( "SMU", "Source Measurement Unit" );
    mAddHead( "Unass1", "Unassigned 1" );
    mAddHead( "Unass2", "Unassigned 2" );
    mAddHead( "Unass3", "Unassigned 3" );
    mAddHead( "Unass4", "Unassigned 4" ); // 95
}


void SEGY::HdrDef::mkBin()
{
    HdrEntry::DataType dtyp = HdrEntry::SInt;
    HdrEntry* he;
    mAddHead4( "jobid", "job identification number" );
    mAddHead4( "lino", "line number" );
    mAddHead4( "reno", "reel number" );
    mAddHead( "ntrpr", "number of data traces per record" );
    mAddHead( "nart", "number of auxiliary traces per record" );

	/* See the remark above in the trace header ns and dt stuff */
    dtyp = HdrEntry::UInt; // entry: 5
    mAddHead( "hdt", "sample interval (micro secs or mm)" );
    mAddHead( "dto", "sample interval for original field recording" );
    mAddHead( "hns", "number of samples per trace" );
    mAddHead( "nso", "#samples per trace for original field recording" );

    dtyp = HdrEntry::SInt; // entry: 9
    mAddHead( "format", "sample format (1,5=float, 3=16 bits, 8=8-bits)" );
    mAddHead( "fold", "CDP fold expected per CDP ensemble" );
    mAddHead( "tsort", "trace sorting code" );
    mAddHead( "vscode", "vertical sum code" );
    mAddHead( "hsfs", "sweep frequency at start" );
    mAddHead( "hsfe", "sweep frequency at end" );
    mAddHead( "hslen", "sweep length (ms)" );
    mAddHead( "hstyp", "sweep type code" );
    mAddHead( "schn", "trace number of sweep channel" );
    mAddHead( "hstas", "sweep trace taper length at start" );
    mAddHead( "hstae", "sweep trace taper length at end" );
    mAddHead( "htatyp", "sweep trace taper type code" ); // entry: 20
    mAddHead( "hcorr", "correlated data traces code" );
    mAddHead( "bgrcv", "binary gain recovered code" );
    mAddHead( "rcvm", "amplitude recovery method code" );
    mAddHead( "mfeet", "measurement system code (1=m 2=ft)" );
    mAddHead( "polyt", "impulse signal polarity code" );
    mAddHead( "vpol", "vibratory polarity code" );
    // entries 0-26 done

    int iunass = 0;
    for ( int bytenr=60; bytenr<400; bytenr+=2 )
    {
	if ( bytenr < 300 || bytenr > 304 )
	{
	    iunass++;
	    BufferString nm( "unass", iunass );
	    mAddHead( nm.buf(), "non-standard / reserved" );
	}
	else
	{
	    dtyp = HdrEntry::UInt; // entry 147
	    mAddHead( "RevCode", "SEG-Y revision code (Rev1=256)" );
	    mAddHead( "FixedSize", "Fixed trace size (1=all traces equal)" );
	    dtyp = HdrEntry::SInt;
	    mAddHead( "NrStanzas", "Number of extra headers ('stanzas')" );
	    bytenr += 4;
	}
    }
}


SEGY::HdrDef::HdrDef( bool binhead )
    : isbin_(binhead)
{
    if ( isbin_ )
	mkBin();
    else
	mkTrc();

    HdrEntry::BytePos bytnr = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	HdrEntry& entry = const_cast<HdrEntry&>( *(*this)[idx] );
	entry.bytepos_ = bytnr;
	bytnr += entry.byteSize();
    }
}


int SEGY::HdrDef::indexOf( const char* nm ) const
{
    if ( !nm || !*nm ) return -1;

    BufferString srchnm( nm );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( srchnm == (*this)[idx]->name() )
	    return idx;
    }

    return -1;
}


int SEGY::HdrDef::idxOfBytePos( SEGY::HdrEntry::BytePos bp,
				unsigned char& offs ) const
{
    offs = 0;
    for ( int idx=bp/4; idx<size(); idx++ )
    {
	const HdrEntry& he = *(*this)[idx];
	if ( he.bytepos_ == bp )
	    return idx;
	else if ( he.bytepos_ > bp )
	{
	    if ( idx )
		offs = mCast( unsigned char, bp - (*this)[idx-1]->bytepos_ );
	    return idx;
	}
    }
    return -1;
}


void SEGY::HdrDef::swapValues( unsigned char* buf ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const HdrEntry& he = *(*this)[idx];
	he.putValue( buf, he.getValue(buf,true) );
    }
}


SEGY::TrcHeaderDef::TrcHeaderDef()
    : inl_(9), crl_(21), xcoord_(73), ycoord_(77), trnr_(5), offs_(37)
{
    azim_.setUdf(); pick_.setUdf(); refnr_.setUdf();

#   define mDefHdr(hdr,nm,desc) hdr##_.setName(nm); hdr##_.setDescription(desc)
    mDefHdr(inl,"In-line","Inline number");
    mDefHdr(crl,"Cross-line","Crossline number");
    mDefHdr(xcoord,"X-Coordinate","X coordinate");
    mDefHdr(ycoord,"Y-Coordinate","Y coordinate");
    mDefHdr(trnr,"Trace Number","Trace identification number");
    mDefHdr(offs,"Offset","Distance between source and receiver");
    mDefHdr(azim,"Azimuth","Direction of line between source and receiver");
    mDefHdr(pick,"Pick","Picked Z position");
    mDefHdr(refnr,"Ref/SP Number","Auxiliary number for trace identification");
}


void SEGY::TrcHeaderDef::usePar( const IOPar& iopar )
{
#define mSgyKey(nm) s##nm##Byte()
    inl_.usePar( iopar, mSgyKey(Inl) );
    crl_.usePar( iopar, mSgyKey(Crl) );
    xcoord_.usePar( iopar, mSgyKey(XCoord) );
    ycoord_.usePar( iopar, mSgyKey(YCoord) );
    offs_.usePar( iopar, mSgyKey(Offs) );
    azim_.usePar( iopar, mSgyKey(Azim) );
    trnr_.usePar( iopar, mSgyKey(TrNr) );
    refnr_.usePar( iopar, mSgyKey(RefNr) );
    pick_.usePar( iopar, mSgyKey(Pick) );
}


void SEGY::TrcHeaderDef::fromSettings()
{
    const IOPar* useiop = &Settings::common();
    IOPar* subiop = useiop->subselect( "SEG-Y" );
    if ( subiop && subiop->size() )
	useiop = subiop;
    usePar( *useiop );
    delete subiop;
}


void SEGY::TrcHeaderDef::fillPar( IOPar& iopar, const char* ky ) const
{
#undef mSgyKey
#define mSgyKey(nm) IOPar::compKey(ky,s##nm##Byte())
    inl_.fillPar( iopar, mSgyKey(Inl) );
    crl_.fillPar( iopar, mSgyKey(Crl) );
    xcoord_.fillPar( iopar, mSgyKey(XCoord) );
    ycoord_.fillPar( iopar, mSgyKey(YCoord) );
    offs_.fillPar( iopar, mSgyKey(Offs) );
    azim_.fillPar( iopar, mSgyKey(Azim) );
    trnr_.fillPar( iopar, mSgyKey(TrNr) );
    refnr_.fillPar( iopar, mSgyKey(RefNr) );
    pick_.fillPar( iopar, mSgyKey(Pick) );
}
