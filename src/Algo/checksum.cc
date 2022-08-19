/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "checksum.h"
#include "legal.h"

/*
 * Redis uses the CRC64 variant with "Jones" coefficients and init value of 0.
 *
 * Specification of this CRC64 variant follows:
 * Name: crc-64-jones
 * Width: 64 bites
 * Poly: 0xad93d23594c935a9
 * Reflected In: True
 * Xor_In: 0xffffffffffffffff
 * Reflected_Out: True
 * Xor_Out: 0x0
 * Check("123456789"): 0xe9c6d914c4b8d9ca " */

static uiString* legalText()
{
    return new uiString(toUiString(
" Copyright (c) 2012, Salvatore Sanfilippo <antirez at gmail dot com> \n"
" All rights reserved. \n"
" \n"
" Redistribution and use in source and binary forms, with or without \n"
" modification, are permitted provided that the following conditions are met:\n"
" \n"
"   * Redistributions of source code must retain the above copyright notice, \n"
"     this list of conditions and the following disclaimer. \n"
"   * Redistributions in binary form must reproduce the above copyright \n"
"     notice, this list of conditions and the following disclaimer in the \n"
"     documentation and/or other materials provided with the distribution. \n"
"   * Neither the name of Redis nor the names of its contributors may be used\n"
"     to endorse or promote products derived from this software without \n"
"     specific prior written permission. \n"
" \n"
" THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'\n"
" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE \n"
" IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE \n"
" ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE \n"
" LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR \n"
" CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF \n"
" SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS \n"
" INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN \n"
" CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) \n"
" ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE \n"
" POSSIBILITY OF SUCH DAMAGE. */ \n" ) );
}

#include <stdint.h>
#include <odver.h>

static const uint64_t crc64_tab[256] = {
    od_uint64(0x0000000000000000), od_uint64(0x7ad870c830358979ULL),
    od_uint64(0xf5b0e190606b12f2ULL), od_uint64(0x8f689158505e9b8bULL),
    od_uint64(0xc038e5739841b68fULL), od_uint64(0xbae095bba8743ff6ULL),
    od_uint64(0x358804e3f82aa47dULL), od_uint64(0x4f50742bc81f2d04ULL),
    od_uint64(0xab28ecb46814fe75ULL), od_uint64(0xd1f09c7c5821770cULL),
    od_uint64(0x5e980d24087fec87ULL), od_uint64(0x24407dec384a65feULL),
    od_uint64(0x6b1009c7f05548faULL), od_uint64(0x11c8790fc060c183ULL),
    od_uint64(0x9ea0e857903e5a08ULL), od_uint64(0xe478989fa00bd371ULL),
    od_uint64(0x7d08ff3b88be6f81ULL), od_uint64(0x07d08ff3b88be6f8ULL),
    od_uint64(0x88b81eabe8d57d73ULL), od_uint64(0xf2606e63d8e0f40aULL),
    od_uint64(0xbd301a4810ffd90eULL), od_uint64(0xc7e86a8020ca5077ULL),
    od_uint64(0x4880fbd87094cbfcULL), od_uint64(0x32588b1040a14285ULL),
    od_uint64(0xd620138fe0aa91f4ULL), od_uint64(0xacf86347d09f188dULL),
    od_uint64(0x2390f21f80c18306ULL), od_uint64(0x594882d7b0f40a7fULL),
    od_uint64(0x1618f6fc78eb277bULL), od_uint64(0x6cc0863448deae02ULL),
    od_uint64(0xe3a8176c18803589ULL), od_uint64(0x997067a428b5bcf0ULL),
    od_uint64(0xfa11fe77117cdf02ULL), od_uint64(0x80c98ebf2149567bULL),
    od_uint64(0x0fa11fe77117cdf0ULL), od_uint64(0x75796f2f41224489ULL),
    od_uint64(0x3a291b04893d698dULL), od_uint64(0x40f16bccb908e0f4ULL),
    od_uint64(0xcf99fa94e9567b7fULL), od_uint64(0xb5418a5cd963f206ULL),
    od_uint64(0x513912c379682177ULL), od_uint64(0x2be1620b495da80eULL),
    od_uint64(0xa489f35319033385ULL), od_uint64(0xde51839b2936bafcULL),
    od_uint64(0x9101f7b0e12997f8ULL), od_uint64(0xebd98778d11c1e81ULL),
    od_uint64(0x64b116208142850aULL), od_uint64(0x1e6966e8b1770c73ULL),
    od_uint64(0x8719014c99c2b083ULL), od_uint64(0xfdc17184a9f739faULL),
    od_uint64(0x72a9e0dcf9a9a271ULL), od_uint64(0x08719014c99c2b08ULL),
    od_uint64(0x4721e43f0183060cULL), od_uint64(0x3df994f731b68f75ULL),
    od_uint64(0xb29105af61e814feULL), od_uint64(0xc849756751dd9d87ULL),
    od_uint64(0x2c31edf8f1d64ef6ULL), od_uint64(0x56e99d30c1e3c78fULL),
    od_uint64(0xd9810c6891bd5c04ULL), od_uint64(0xa3597ca0a188d57dULL),
    od_uint64(0xec09088b6997f879ULL), od_uint64(0x96d1784359a27100ULL),
    od_uint64(0x19b9e91b09fcea8bULL), od_uint64(0x636199d339c963f2ULL),
    od_uint64(0xdf7adabd7a6e2d6fULL), od_uint64(0xa5a2aa754a5ba416ULL),
    od_uint64(0x2aca3b2d1a053f9dULL), od_uint64(0x50124be52a30b6e4ULL),
    od_uint64(0x1f423fcee22f9be0ULL), od_uint64(0x659a4f06d21a1299ULL),
    od_uint64(0xeaf2de5e82448912ULL), od_uint64(0x902aae96b271006bULL),
    od_uint64(0x74523609127ad31aULL), od_uint64(0x0e8a46c1224f5a63ULL),
    od_uint64(0x81e2d7997211c1e8ULL), od_uint64(0xfb3aa75142244891ULL),
    od_uint64(0xb46ad37a8a3b6595ULL), od_uint64(0xceb2a3b2ba0eececULL),
    od_uint64(0x41da32eaea507767ULL), od_uint64(0x3b024222da65fe1eULL),
    od_uint64(0xa2722586f2d042eeULL), od_uint64(0xd8aa554ec2e5cb97ULL),
    od_uint64(0x57c2c41692bb501cULL), od_uint64(0x2d1ab4dea28ed965ULL),
    od_uint64(0x624ac0f56a91f461ULL), od_uint64(0x1892b03d5aa47d18ULL),
    od_uint64(0x97fa21650afae693ULL), od_uint64(0xed2251ad3acf6feaULL),
    od_uint64(0x095ac9329ac4bc9bULL), od_uint64(0x7382b9faaaf135e2ULL),
    od_uint64(0xfcea28a2faafae69ULL), od_uint64(0x8632586aca9a2710ULL),
    od_uint64(0xc9622c4102850a14ULL), od_uint64(0xb3ba5c8932b0836dULL),
    od_uint64(0x3cd2cdd162ee18e6ULL), od_uint64(0x460abd1952db919fULL),
    od_uint64(0x256b24ca6b12f26dULL), od_uint64(0x5fb354025b277b14ULL),
    od_uint64(0xd0dbc55a0b79e09fULL), od_uint64(0xaa03b5923b4c69e6ULL),
    od_uint64(0xe553c1b9f35344e2ULL), od_uint64(0x9f8bb171c366cd9bULL),
    od_uint64(0x10e3202993385610ULL), od_uint64(0x6a3b50e1a30ddf69ULL),
    od_uint64(0x8e43c87e03060c18ULL), od_uint64(0xf49bb8b633338561ULL),
    od_uint64(0x7bf329ee636d1eeaULL), od_uint64(0x012b592653589793ULL),
    od_uint64(0x4e7b2d0d9b47ba97ULL), od_uint64(0x34a35dc5ab7233eeULL),
    od_uint64(0xbbcbcc9dfb2ca865ULL), od_uint64(0xc113bc55cb19211cULL),
    od_uint64(0x5863dbf1e3ac9decULL), od_uint64(0x22bbab39d3991495ULL),
    od_uint64(0xadd33a6183c78f1eULL), od_uint64(0xd70b4aa9b3f20667ULL),
    od_uint64(0x985b3e827bed2b63ULL), od_uint64(0xe2834e4a4bd8a21aULL),
    od_uint64(0x6debdf121b863991ULL), od_uint64(0x1733afda2bb3b0e8ULL),
    od_uint64(0xf34b37458bb86399ULL), od_uint64(0x8993478dbb8deae0ULL),
    od_uint64(0x06fbd6d5ebd3716bULL), od_uint64(0x7c23a61ddbe6f812ULL),
    od_uint64(0x3373d23613f9d516ULL), od_uint64(0x49aba2fe23cc5c6fULL),
    od_uint64(0xc6c333a67392c7e4ULL), od_uint64(0xbc1b436e43a74e9dULL),
    od_uint64(0x95ac9329ac4bc9b5ULL), od_uint64(0xef74e3e19c7e40ccULL),
    od_uint64(0x601c72b9cc20db47ULL), od_uint64(0x1ac40271fc15523eULL),
    od_uint64(0x5594765a340a7f3aULL), od_uint64(0x2f4c0692043ff643ULL),
    od_uint64(0xa02497ca54616dc8ULL), od_uint64(0xdafce7026454e4b1ULL),
    od_uint64(0x3e847f9dc45f37c0ULL), od_uint64(0x445c0f55f46abeb9ULL),
    od_uint64(0xcb349e0da4342532ULL), od_uint64(0xb1eceec59401ac4bULL),
    od_uint64(0xfebc9aee5c1e814fULL), od_uint64(0x8464ea266c2b0836ULL),
    od_uint64(0x0b0c7b7e3c7593bdULL), od_uint64(0x71d40bb60c401ac4ULL),
    od_uint64(0xe8a46c1224f5a634ULL), od_uint64(0x927c1cda14c02f4dULL),
    od_uint64(0x1d148d82449eb4c6ULL), od_uint64(0x67ccfd4a74ab3dbfULL),
    od_uint64(0x289c8961bcb410bbULL), od_uint64(0x5244f9a98c8199c2ULL),
    od_uint64(0xdd2c68f1dcdf0249ULL), od_uint64(0xa7f41839ecea8b30ULL),
    od_uint64(0x438c80a64ce15841ULL), od_uint64(0x3954f06e7cd4d138ULL),
    od_uint64(0xb63c61362c8a4ab3ULL), od_uint64(0xcce411fe1cbfc3caULL),
    od_uint64(0x83b465d5d4a0eeceULL), od_uint64(0xf96c151de49567b7ULL),
    od_uint64(0x76048445b4cbfc3cULL), od_uint64(0x0cdcf48d84fe7545ULL),
    od_uint64(0x6fbd6d5ebd3716b7ULL), od_uint64(0x15651d968d029fceULL),
    od_uint64(0x9a0d8ccedd5c0445ULL), od_uint64(0xe0d5fc06ed698d3cULL),
    od_uint64(0xaf85882d2576a038ULL), od_uint64(0xd55df8e515432941ULL),
    od_uint64(0x5a3569bd451db2caULL), od_uint64(0x20ed197575283bb3ULL),
    od_uint64(0xc49581ead523e8c2ULL), od_uint64(0xbe4df122e51661bbULL),
    od_uint64(0x3125607ab548fa30ULL), od_uint64(0x4bfd10b2857d7349ULL),
    od_uint64(0x04ad64994d625e4dULL), od_uint64(0x7e7514517d57d734ULL),
    od_uint64(0xf11d85092d094cbfULL), od_uint64(0x8bc5f5c11d3cc5c6ULL),
    od_uint64(0x12b5926535897936ULL), od_uint64(0x686de2ad05bcf04fULL),
    od_uint64(0xe70573f555e26bc4ULL), od_uint64(0x9ddd033d65d7e2bdULL),
    od_uint64(0xd28d7716adc8cfb9ULL), od_uint64(0xa85507de9dfd46c0ULL),
    od_uint64(0x273d9686cda3dd4bULL), od_uint64(0x5de5e64efd965432ULL),
    od_uint64(0xb99d7ed15d9d8743ULL), od_uint64(0xc3450e196da80e3aULL),
    od_uint64(0x4c2d9f413df695b1ULL), od_uint64(0x36f5ef890dc31cc8ULL),
    od_uint64(0x79a59ba2c5dc31ccULL), od_uint64(0x037deb6af5e9b8b5ULL),
    od_uint64(0x8c157a32a5b7233eULL), od_uint64(0xf6cd0afa9582aa47ULL),
    od_uint64(0x4ad64994d625e4daULL), od_uint64(0x300e395ce6106da3ULL),
    od_uint64(0xbf66a804b64ef628ULL), od_uint64(0xc5bed8cc867b7f51ULL),
    od_uint64(0x8aeeace74e645255ULL), od_uint64(0xf036dc2f7e51db2cULL),
    od_uint64(0x7f5e4d772e0f40a7ULL), od_uint64(0x05863dbf1e3ac9deULL),
    od_uint64(0xe1fea520be311aafULL), od_uint64(0x9b26d5e88e0493d6ULL),
    od_uint64(0x144e44b0de5a085dULL), od_uint64(0x6e963478ee6f8124ULL),
    od_uint64(0x21c640532670ac20ULL), od_uint64(0x5b1e309b16452559ULL),
    od_uint64(0xd476a1c3461bbed2ULL), od_uint64(0xaeaed10b762e37abULL),
    od_uint64(0x37deb6af5e9b8b5bULL), od_uint64(0x4d06c6676eae0222ULL),
    od_uint64(0xc26e573f3ef099a9ULL), od_uint64(0xb8b627f70ec510d0ULL),
    od_uint64(0xf7e653dcc6da3dd4ULL), od_uint64(0x8d3e2314f6efb4adULL),
    od_uint64(0x0256b24ca6b12f26ULL), od_uint64(0x788ec2849684a65fULL),
    od_uint64(0x9cf65a1b368f752eULL), od_uint64(0xe62e2ad306bafc57ULL),
    od_uint64(0x6946bb8b56e467dcULL), od_uint64(0x139ecb4366d1eea5ULL),
    od_uint64(0x5ccebf68aecec3a1ULL), od_uint64(0x2616cfa09efb4ad8ULL),
    od_uint64(0xa97e5ef8cea5d153ULL), od_uint64(0xd3a62e30fe90582aULL),
    od_uint64(0xb0c7b7e3c7593bd8ULL), od_uint64(0xca1fc72bf76cb2a1ULL),
    od_uint64(0x45775673a732292aULL), od_uint64(0x3faf26bb9707a053ULL),
    od_uint64(0x70ff52905f188d57ULL), od_uint64(0x0a2722586f2d042eULL),
    od_uint64(0x854fb3003f739fa5ULL), od_uint64(0xff97c3c80f4616dcULL),
    od_uint64(0x1bef5b57af4dc5adULL), od_uint64(0x61372b9f9f784cd4ULL),
    od_uint64(0xee5fbac7cf26d75fULL), od_uint64(0x9487ca0fff135e26ULL),
    od_uint64(0xdbd7be24370c7322ULL), od_uint64(0xa10fceec0739fa5bULL),
    od_uint64(0x2e675fb4576761d0ULL), od_uint64(0x54bf2f7c6752e8a9ULL),
    od_uint64(0xcdcf48d84fe75459ULL), od_uint64(0xb71738107fd2dd20ULL),
    od_uint64(0x387fa9482f8c46abULL), od_uint64(0x42a7d9801fb9cfd2ULL),
    od_uint64(0x0df7adabd7a6e2d6ULL), od_uint64(0x772fdd63e7936bafULL),
    od_uint64(0xf8474c3bb7cdf024ULL), od_uint64(0x829f3cf387f8795dULL),
    od_uint64(0x66e7a46c27f3aa2cULL), od_uint64(0x1c3fd4a417c62355ULL),
    od_uint64(0x935745fc4798b8deULL), od_uint64(0xe98f353477ad31a7ULL),
    od_uint64(0xa6df411fbfb21ca3ULL), od_uint64(0xdc0731d78f8795daULL),
    od_uint64(0x536fa08fdfd90e51ULL), od_uint64(0x29b7d047efec8728ULL),
};


void initChecksum()
{
    legalInformation().addCreator( legalText, "CRC64 Checksum" );
}

od_uint64 checksum64( const unsigned char *s, od_uint64 l, od_uint64 crc )
{
    for ( od_int64 j=0; j<l; j++ )
    {
	unsigned char byte = s[j];
	crc = crc64_tab[(uint8_t)crc ^ byte] ^ (crc >> 8);
    }

    return crc;
}
