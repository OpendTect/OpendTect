#ifndef segyhdrdef_h
#define segyhdrdef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "objectset.h"


namespace SEGY
{

class HdrDef;

/*!\brief describes one tape or trace header field,

Note: it is important to know whether your HdrEntry is in 'internal' or in
'user' format. The difference is 1 byte. For example, tracr is at byte 4 of
the trace header, but users need to see 5.
The reasone we don't always use internal entries is that some things have to
be directly stored in user settings.
 */

mExpClass(Seis) HdrEntry
{
public:

    typedef short	BytePos;
    enum DataType	{ SInt, UInt, Float };
			//!< Note that Float is against the standard

			HdrEntry( BytePos bp=udfBP(), bool issmll=false,
				  DataType dt=SInt )
			    : bytepos_(bp), issmall_(issmll), type_(dt)
			    , desc_(0), name_(0)	{}
			HdrEntry( const HdrEntry& he )
			    : desc_(0), name_(0)	{ *this = he; }
			~HdrEntry() { delete [] desc_; delete [] name_; }
    HdrEntry&		operator =(const HdrEntry&);

    const char*		description() const;
    void		setDescription(const char*);
    const char*		name() const;
    void		setName(const char*);

    BytePos		bytepos_;
    bool		issmall_;
    DataType		type_;

    bool		isInternal() const	{ return bytepos_%2 == 0; }
    int			byteSize() const	{ return issmall_ ? 2 : 4; }
    inline bool		isUdf() const		{ return bytepos_ < 0; }
    inline void		setUdf()		{ bytepos_ = udfBP(); }
    inline bool		usesByte( BytePos b ) const
			{ return b >= bytepos_ && b < bytepos_ + byteSize(); }

    int			getValue(const void* buf,bool swapped=false) const;
    void		putValue(void* buf,int) const;

    void		usePar(const IOPar&,const char* ky,const HdrDef* hd=0);
    void		fillPar(IOPar&,const char* ky) const;
    void		removeFromPar(IOPar&,const char* ky) const;

protected:

    char*		desc_;
    char*		name_;

    inline static	BytePos udfBP()		{ return -32768; }

};


mExpClass(Seis) HdrDef : public ObjectSet<const HdrEntry>
{
public:

			HdrDef(bool binhead);
    bool		isBin() const		{ return isbin_; }

    int			indexOf(const char* nm) const;
    int			idxOfBytePos(HdrEntry::BytePos,
				     unsigned char& offs) const;

    void		swapValues(unsigned char*) const;

    int			indexOf( const HdrEntry* he ) const
			{ return ObjectSet<const HdrEntry>::indexOf(he); }

protected:

    bool		isbin_;

    void		mkTrc();
    void		mkBin();

};


} // namespace

#endif
