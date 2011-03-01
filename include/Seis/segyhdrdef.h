#ifndef segythentry_h
#define segythentry_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2011
 RCS:		$Id: segyhdrdef.h,v 1.2 2011-03-01 11:40:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "objectset.h"
class IOPar;
 
namespace SEGY
{

/*!\brief describes one tape or trace header field */

mClass HdrEntry
{
public:

    typedef short	BytePos;
    enum DataType	{ SInt, UInt, Float };
    			//!< Note that Float is against the standard

			HdrEntry( BytePos bp=udfBP(), bool issmall=false,
				  DataType dt=SInt )
			    : bytepos_(bp)
			    , small_(issmall)
			    , type_(dt)
			    , desc_(0)
    			    , name_(0)	{}

    const char*		description() const;
    void		setDescription(const char*);
    const char*		name() const;
    void		setName(const char*);

    BytePos		bytepos_;
    bool		small_;
    DataType		type_;

    int			byteSize() const	{ return small_ ? 2 : 4; }
    inline bool		isUdf() const		{ return bytepos_ < 0; }
    inline void		setUdf()		{ bytepos_ = udfBP(); }
    inline bool		usesByte( BytePos b ) const
			{ return b >= bytepos_ && b < bytepos_ + byteSize(); }

    int			getValue(const void* buf,bool swapped=false) const;
    void		putValue(void* buf,int) const;

    void		usePar(const IOPar&,const char* ky);
    void		fillPar(IOPar&,const char* ky) const;
    void		removeFromPar(IOPar&,const char* ky) const;

protected:

    char*		desc_;
    char*		name_;

    inline static const BytePos udfBP()		{ return -32768; }

};


mClass HdrDef : public ObjectSet<const HdrEntry>
{
public:

    			HdrDef(bool binhead);
    bool		isBin() const		{ return isbin_; }

    int			idxOfBytePos(HdrEntry::BytePos,
	    			     unsigned char& offs) const;

    void		swapValues(unsigned char*) const;

protected:

    bool		isbin_;

    void		mkTrc();
    void		mkBin();

};

} // namespace

#endif
