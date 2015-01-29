/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2015
-*/

static const char* rcsID mUsedVar = "$Id: probdenfunc.cc 36757 2014-10-03 15:48:56Z bert.bril@dgbes.com $";


#include "checksum.h"
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
 * Check("123456789"): 0xe9c6d914c4b8d9ca
 *
 * Copyright (c) 2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. */
 
#include <stdint.h>

static const uint64_t crc64_tab[256] = {
    od_uint64(0x0000000000000000), od_uint64(0x7ad870c830358979),
    od_uint64(0xf5b0e190606b12f2), od_uint64(0x8f689158505e9b8b),
    od_uint64(0xc038e5739841b68f), od_uint64(0xbae095bba8743ff6),
    od_uint64(0x358804e3f82aa47d), od_uint64(0x4f50742bc81f2d04),
    od_uint64(0xab28ecb46814fe75), od_uint64(0xd1f09c7c5821770c),
    od_uint64(0x5e980d24087fec87), od_uint64(0x24407dec384a65fe),
    od_uint64(0x6b1009c7f05548fa), od_uint64(0x11c8790fc060c183),
    od_uint64(0x9ea0e857903e5a08), od_uint64(0xe478989fa00bd371),
    od_uint64(0x7d08ff3b88be6f81), od_uint64(0x07d08ff3b88be6f8),
    od_uint64(0x88b81eabe8d57d73), od_uint64(0xf2606e63d8e0f40a),
    od_uint64(0xbd301a4810ffd90e), od_uint64(0xc7e86a8020ca5077),
    od_uint64(0x4880fbd87094cbfc), od_uint64(0x32588b1040a14285),
    od_uint64(0xd620138fe0aa91f4), od_uint64(0xacf86347d09f188d),
    od_uint64(0x2390f21f80c18306), od_uint64(0x594882d7b0f40a7f),
    od_uint64(0x1618f6fc78eb277b), od_uint64(0x6cc0863448deae02),
    od_uint64(0xe3a8176c18803589), od_uint64(0x997067a428b5bcf0),
    od_uint64(0xfa11fe77117cdf02), od_uint64(0x80c98ebf2149567b),
    od_uint64(0x0fa11fe77117cdf0), od_uint64(0x75796f2f41224489),
    od_uint64(0x3a291b04893d698d), od_uint64(0x40f16bccb908e0f4),
    od_uint64(0xcf99fa94e9567b7f), od_uint64(0xb5418a5cd963f206),
    od_uint64(0x513912c379682177), od_uint64(0x2be1620b495da80e),
    od_uint64(0xa489f35319033385), od_uint64(0xde51839b2936bafc),
    od_uint64(0x9101f7b0e12997f8), od_uint64(0xebd98778d11c1e81),
    od_uint64(0x64b116208142850a), od_uint64(0x1e6966e8b1770c73),
    od_uint64(0x8719014c99c2b083), od_uint64(0xfdc17184a9f739fa),
    od_uint64(0x72a9e0dcf9a9a271), od_uint64(0x08719014c99c2b08),
    od_uint64(0x4721e43f0183060c), od_uint64(0x3df994f731b68f75),
    od_uint64(0xb29105af61e814fe), od_uint64(0xc849756751dd9d87),
    od_uint64(0x2c31edf8f1d64ef6), od_uint64(0x56e99d30c1e3c78f),
    od_uint64(0xd9810c6891bd5c04), od_uint64(0xa3597ca0a188d57d),
    od_uint64(0xec09088b6997f879), od_uint64(0x96d1784359a27100),
    od_uint64(0x19b9e91b09fcea8b), od_uint64(0x636199d339c963f2),
    od_uint64(0xdf7adabd7a6e2d6f), od_uint64(0xa5a2aa754a5ba416),
    od_uint64(0x2aca3b2d1a053f9d), od_uint64(0x50124be52a30b6e4),
    od_uint64(0x1f423fcee22f9be0), od_uint64(0x659a4f06d21a1299),
    od_uint64(0xeaf2de5e82448912), od_uint64(0x902aae96b271006b),
    od_uint64(0x74523609127ad31a), od_uint64(0x0e8a46c1224f5a63),
    od_uint64(0x81e2d7997211c1e8), od_uint64(0xfb3aa75142244891),
    od_uint64(0xb46ad37a8a3b6595), od_uint64(0xceb2a3b2ba0eecec),
    od_uint64(0x41da32eaea507767), od_uint64(0x3b024222da65fe1e),
    od_uint64(0xa2722586f2d042ee), od_uint64(0xd8aa554ec2e5cb97),
    od_uint64(0x57c2c41692bb501c), od_uint64(0x2d1ab4dea28ed965),
    od_uint64(0x624ac0f56a91f461), od_uint64(0x1892b03d5aa47d18),
    od_uint64(0x97fa21650afae693), od_uint64(0xed2251ad3acf6fea),
    od_uint64(0x095ac9329ac4bc9b), od_uint64(0x7382b9faaaf135e2),
    od_uint64(0xfcea28a2faafae69), od_uint64(0x8632586aca9a2710),
    od_uint64(0xc9622c4102850a14), od_uint64(0xb3ba5c8932b0836d),
    od_uint64(0x3cd2cdd162ee18e6), od_uint64(0x460abd1952db919f),
    od_uint64(0x256b24ca6b12f26d), od_uint64(0x5fb354025b277b14),
    od_uint64(0xd0dbc55a0b79e09f), od_uint64(0xaa03b5923b4c69e6),
    od_uint64(0xe553c1b9f35344e2), od_uint64(0x9f8bb171c366cd9b),
    od_uint64(0x10e3202993385610), od_uint64(0x6a3b50e1a30ddf69),
    od_uint64(0x8e43c87e03060c18), od_uint64(0xf49bb8b633338561),
    od_uint64(0x7bf329ee636d1eea), od_uint64(0x012b592653589793),
    od_uint64(0x4e7b2d0d9b47ba97), od_uint64(0x34a35dc5ab7233ee),
    od_uint64(0xbbcbcc9dfb2ca865), od_uint64(0xc113bc55cb19211c),
    od_uint64(0x5863dbf1e3ac9dec), od_uint64(0x22bbab39d3991495),
    od_uint64(0xadd33a6183c78f1e), od_uint64(0xd70b4aa9b3f20667),
    od_uint64(0x985b3e827bed2b63), od_uint64(0xe2834e4a4bd8a21a),
    od_uint64(0x6debdf121b863991), od_uint64(0x1733afda2bb3b0e8),
    od_uint64(0xf34b37458bb86399), od_uint64(0x8993478dbb8deae0),
    od_uint64(0x06fbd6d5ebd3716b), od_uint64(0x7c23a61ddbe6f812),
    od_uint64(0x3373d23613f9d516), od_uint64(0x49aba2fe23cc5c6f),
    od_uint64(0xc6c333a67392c7e4), od_uint64(0xbc1b436e43a74e9d),
    od_uint64(0x95ac9329ac4bc9b5), od_uint64(0xef74e3e19c7e40cc),
    od_uint64(0x601c72b9cc20db47), od_uint64(0x1ac40271fc15523e),
    od_uint64(0x5594765a340a7f3a), od_uint64(0x2f4c0692043ff643),
    od_uint64(0xa02497ca54616dc8), od_uint64(0xdafce7026454e4b1),
    od_uint64(0x3e847f9dc45f37c0), od_uint64(0x445c0f55f46abeb9),
    od_uint64(0xcb349e0da4342532), od_uint64(0xb1eceec59401ac4b),
    od_uint64(0xfebc9aee5c1e814f), od_uint64(0x8464ea266c2b0836),
    od_uint64(0x0b0c7b7e3c7593bd), od_uint64(0x71d40bb60c401ac4),
    od_uint64(0xe8a46c1224f5a634), od_uint64(0x927c1cda14c02f4d),
    od_uint64(0x1d148d82449eb4c6), od_uint64(0x67ccfd4a74ab3dbf),
    od_uint64(0x289c8961bcb410bb), od_uint64(0x5244f9a98c8199c2),
    od_uint64(0xdd2c68f1dcdf0249), od_uint64(0xa7f41839ecea8b30),
    od_uint64(0x438c80a64ce15841), od_uint64(0x3954f06e7cd4d138),
    od_uint64(0xb63c61362c8a4ab3), od_uint64(0xcce411fe1cbfc3ca),
    od_uint64(0x83b465d5d4a0eece), od_uint64(0xf96c151de49567b7),
    od_uint64(0x76048445b4cbfc3c), od_uint64(0x0cdcf48d84fe7545),
    od_uint64(0x6fbd6d5ebd3716b7), od_uint64(0x15651d968d029fce),
    od_uint64(0x9a0d8ccedd5c0445), od_uint64(0xe0d5fc06ed698d3c),
    od_uint64(0xaf85882d2576a038), od_uint64(0xd55df8e515432941),
    od_uint64(0x5a3569bd451db2ca), od_uint64(0x20ed197575283bb3),
    od_uint64(0xc49581ead523e8c2), od_uint64(0xbe4df122e51661bb),
    od_uint64(0x3125607ab548fa30), od_uint64(0x4bfd10b2857d7349),
    od_uint64(0x04ad64994d625e4d), od_uint64(0x7e7514517d57d734),
    od_uint64(0xf11d85092d094cbf), od_uint64(0x8bc5f5c11d3cc5c6),
    od_uint64(0x12b5926535897936), od_uint64(0x686de2ad05bcf04f),
    od_uint64(0xe70573f555e26bc4), od_uint64(0x9ddd033d65d7e2bd),
    od_uint64(0xd28d7716adc8cfb9), od_uint64(0xa85507de9dfd46c0),
    od_uint64(0x273d9686cda3dd4b), od_uint64(0x5de5e64efd965432),
    od_uint64(0xb99d7ed15d9d8743), od_uint64(0xc3450e196da80e3a),
    od_uint64(0x4c2d9f413df695b1), od_uint64(0x36f5ef890dc31cc8),
    od_uint64(0x79a59ba2c5dc31cc), od_uint64(0x037deb6af5e9b8b5),
    od_uint64(0x8c157a32a5b7233e), od_uint64(0xf6cd0afa9582aa47),
    od_uint64(0x4ad64994d625e4da), od_uint64(0x300e395ce6106da3),
    od_uint64(0xbf66a804b64ef628), od_uint64(0xc5bed8cc867b7f51),
    od_uint64(0x8aeeace74e645255), od_uint64(0xf036dc2f7e51db2c),
    od_uint64(0x7f5e4d772e0f40a7), od_uint64(0x05863dbf1e3ac9de),
    od_uint64(0xe1fea520be311aaf), od_uint64(0x9b26d5e88e0493d6),
    od_uint64(0x144e44b0de5a085d), od_uint64(0x6e963478ee6f8124),
    od_uint64(0x21c640532670ac20), od_uint64(0x5b1e309b16452559),
    od_uint64(0xd476a1c3461bbed2), od_uint64(0xaeaed10b762e37ab),
    od_uint64(0x37deb6af5e9b8b5b), od_uint64(0x4d06c6676eae0222),
    od_uint64(0xc26e573f3ef099a9), od_uint64(0xb8b627f70ec510d0),
    od_uint64(0xf7e653dcc6da3dd4), od_uint64(0x8d3e2314f6efb4ad),
    od_uint64(0x0256b24ca6b12f26), od_uint64(0x788ec2849684a65f),
    od_uint64(0x9cf65a1b368f752e), od_uint64(0xe62e2ad306bafc57),
    od_uint64(0x6946bb8b56e467dc), od_uint64(0x139ecb4366d1eea5),
    od_uint64(0x5ccebf68aecec3a1), od_uint64(0x2616cfa09efb4ad8),
    od_uint64(0xa97e5ef8cea5d153), od_uint64(0xd3a62e30fe90582a),
    od_uint64(0xb0c7b7e3c7593bd8), od_uint64(0xca1fc72bf76cb2a1),
    od_uint64(0x45775673a732292a), od_uint64(0x3faf26bb9707a053),
    od_uint64(0x70ff52905f188d57), od_uint64(0x0a2722586f2d042e),
    od_uint64(0x854fb3003f739fa5), od_uint64(0xff97c3c80f4616dc),
    od_uint64(0x1bef5b57af4dc5ad), od_uint64(0x61372b9f9f784cd4),
    od_uint64(0xee5fbac7cf26d75f), od_uint64(0x9487ca0fff135e26),
    od_uint64(0xdbd7be24370c7322), od_uint64(0xa10fceec0739fa5b),
    od_uint64(0x2e675fb4576761d0), od_uint64(0x54bf2f7c6752e8a9),
    od_uint64(0xcdcf48d84fe75459), od_uint64(0xb71738107fd2dd20),
    od_uint64(0x387fa9482f8c46ab), od_uint64(0x42a7d9801fb9cfd2),
    od_uint64(0x0df7adabd7a6e2d6), od_uint64(0x772fdd63e7936baf),
    od_uint64(0xf8474c3bb7cdf024), od_uint64(0x829f3cf387f8795d),
    od_uint64(0x66e7a46c27f3aa2c), od_uint64(0x1c3fd4a417c62355),
    od_uint64(0x935745fc4798b8de), od_uint64(0xe98f353477ad31a7),
    od_uint64(0xa6df411fbfb21ca3), od_uint64(0xdc0731d78f8795da),
    od_uint64(0x536fa08fdfd90e51), od_uint64(0x29b7d047efec8728),
};

od_uint64 checksum64( const unsigned char *s, od_uint64 l, od_uint64 crc )
{
    for ( od_int64 j=0; j<l; j++ )
    {
	unsigned char byte = s[j];
	crc = crc64_tab[(uint8_t)crc ^ byte] ^ (crc >> 8);
    }

    return crc;
}

/* Test main */
#ifdef TEST_MAIN
#include <stdio.h>
int main(void) {
        printf("e9c6d914c4b8d9ca == %016llx\n",
		        (unsigned long long) crc64(0,(unsigned char*)"123456789",9));
	    return 0;
}
#endif
