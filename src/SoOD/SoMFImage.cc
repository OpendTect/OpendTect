/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "SoMFImage.h"

#include <Inventor/errors/SoReadError.h>

// bug fix for problem while copying SbImagei32 in setValues
// remove the macros below once the bug is fixed in Coin

// ******************** remove from this point ********************
#undef SO_MFIELD_VALUE_SOURCE

// from SoSubField.h with the only difference:
// ( _valtype_ ) cast removed from newvals[i] in the setValues method
#define SO_MFIELD_VALUE_SOURCE(_class_, _valtype_, _valref_) \
    int \
_class_::fieldSizeof(void) const \
{ \
          return sizeof(_valtype_); \
} \
 \
void * \
_class_::valuesPtr(void) \
{ \
          return (void *)this->values; \
} \
 \
void \
_class_::setValuesPtr(void * ptr) \
{ \
          this->values = (_valtype_ *)ptr; \
} \
 \
int \
_class_::find(_valref_ value, SbBool addifnotfound) \
{ \
          evaluate(); \
          for (int i=0; i < this->num; i++) if (this->values[i] == value) return i; \
         \
          if (addifnotfound) this->set1Value(this->num, value); \
          return -1; \
} \
 \
void \
_class_::setValues(const int start, const int numarg, const _valtype_ * newvals) \
{ \
          if (start+numarg > this->maxNum) this->allocValues(start+numarg); \
          else if (start+numarg > this->num) this->num = start+numarg; \
         \
          for (int i=0; i < numarg; i++) \
            this->values[i+start] = newvals[i]; \
          this->valueChanged(); \
} \
 \
void \
_class_::set1Value(const int idx, _valref_ value) \
{ \
          if (idx+1 > this->maxNum) this->allocValues(idx+1); \
          else if (idx+1 > this->num) this->num = idx+1; \
          this->values[idx] = value; \
          this->valueChanged(); \
} \
 \
void \
_class_::setValue(_valref_ value) \
{ \
          this->allocValues(1); \
          this->values[0] = value; \
          this->valueChanged(); \
} \
 \
SbBool \
_class_::operator==(const _class_ & field) const \
{ \
          if (this == &field) return TRUE; \
          if (this->getNum() != field.getNum()) return FALSE; \
         \
          const _valtype_ * const lhs = this->getValues(0); \
          const _valtype_ * const rhs = field.getValues(0); \
          for (int i = 0; i < this->num; i++) if (lhs[i] != rhs[i]) return FALSE; \
          return TRUE; \
} \
 \
/*! \COININTERNAL */ \
void \
_class_::deleteAllValues(void) \
{ \
          this->setNum(0); \
} \
 \
/*! This method is used for moving values around internally within a multivalue field. It needs to be overridden in each field so it automatically takes care of running copy contructors where necessary. */ \
void \
_class_::copyValue(int to, int from) \
{ \
          this->values[to] = this->values[from]; \
}

// ******************** remove upto here ********************


SO_MFIELD_SOURCE( SoMFImagei32, SbImagei32, const SbImagei32& );


void SoMFImagei32::initClass()
{
    SO_MFIELD_INIT_CLASS( SoMFImagei32, SoMField );
}


#define mErrRet { SoReadError::post(in, prematuremsg ); return false; }

SbBool SoMFImagei32::read1Value( SoInput* in, int index )
{
    SbVec3i32 size;
    int nc;

    static const char* prematuremsg = "Premature end of file";

    if ( !in->read( size[0] ) || !in->read( size[1] ) || !in->read( size[2] ) ||
         !in->read( nc ) )
	mErrRet;

    if ( size[0]<0 || size[1]<0 || size[2]<0 || nc<0 || nc>4 )
    {
	SoReadError::post( in, "Invalid image specification %dx%dx%dx%d",
			   size[0], size[1], size[2], nc );
	return false;
    }

    const double buffersize = size[0] * size[1] * size[2] * nc;

    const int maxint = (int) (pow( 2.0, (int) ((sizeof(int)*8)-1) ) - 1);
    if ( buffersize >= maxint )
    {
	SoReadError::post( in, "Image too large to be handled: %dx%dx%dx%d",
		size[0], size[1], size[2], nc );
	return false;
    }

    if ( !buffersize && (size[0] || size[1] || size[2] || nc) )
    {
	SoReadError::post( in, "Invalid image specification %dx%dx%dx%d",
			   size[0], size[1], size[2], nc );
	return false;
    }


    if ( !buffersize ) 
    {
	values[index].setValue( SbVec3i32(0,0,0), 0, NULL );
	return true;
    }

    values[index].setValue(size, nc, 0 );
    unsigned char* pixblock = values[index].getValue( size, nc );

    if ( in->isBinary() && in->getIVVersion() >= 2.1f)
    {
	if ( !in->readBinaryArray( pixblock, (int) buffersize ) )
	    mErrRet;
    }
    else
    {
	int byte = 0;
	const int numpixels = size[0] * size[1] * size[2];
	for ( int i=0; i<numpixels; i++ )
	{
	    unsigned int l;
	    if ( !in->read( l ) )
		mErrRet;

	    for ( int j=0; j<nc; j++ )
		pixblock[byte++] =
		    (unsigned char) ((l >> (8 * (nc-j-1))) & 0xFF);
	}
    }

    return true;
}


#define mWriteSpace if ( !binary ) out->write(' ')

void SoMFImagei32::write1Value( SoOutput* out, int index ) const
{
    int nc;
    SbVec3i32 size;
    unsigned char * pixblock = values[index].getValue( size, nc );

    const bool binary = out->isBinary();

    if ( !pixblock )
    {
	// Make size 0 and put in stuff anyway. Otherwise, ivfileviewer will 
	// complain when it cannot read the individual images of the texture 
	// channels.
	size = SbVec3i32( 0, 0, 0 );
	nc = 0;
    }

    out->write( size[0] ); mWriteSpace;
    out->write( size[1] ); mWriteSpace;
    out->write( size[2] ); mWriteSpace;
    out->write( nc );

    if ( !pixblock )
	return;

    if ( out->isBinary() )
    {
	int buffersize = size[0] * size[1] * size[2] * nc;
	if ( buffersize )
	{
	    out->writeBinaryArray( pixblock, buffersize );
	    int padsize = ((buffersize + 3) / 4) * 4 - buffersize;
	    if ( padsize )
	    {
		unsigned char pads[3] = {'\0','\0','\0'};
		out->writeBinaryArray( pads, padsize );
	    }
	}
    }
    else
    {
	out->write( '\n' );
	out->indent();

	int numpixels = size[0] * size[1] * size[2];
	for ( int i=0; i<numpixels; i++ )
	{
	    unsigned int data = 0;
	    for ( int j=0; j<nc; j++ )
	    {
		if ( j ) data <<= 8;
		data |= (uint32_t)(pixblock[i * nc + j]);
	    }

	    out->write( data );
	    if ( ((i+1)%8 == 0) && (i+1 != numpixels))
	    {
		out->write( '\n' );
		out->indent();
	    }
	    else
	    {
		out->write( ' ' );
	    }
	}
    }
}

