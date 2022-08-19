#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "networkmod.h"

#include "arrayndimpl.h"
#include "arrayndinfo.h"
#include "bufstringset.h"
#include "commontypes.h"
#include "odjson.h"
#include "pythonaccess.h"
#include "refcount.h"
#include "uistringset.h"

#define mRequestPacketHeaderSize		10

class IOPar;
template <class T> class ArrayND;


namespace Network
{

class PacketFiller;
class PacketInterpreter;

/*\brief Standardized packet that can be sent over a Tcp connection

  The request header contains a unique (int) request ID and a (short) sub-ID.
  The sub-ID is a flag that can be used to implement your protocol. Negative
  sub-IDs are special and cannot be freely used.

  Servers will not send Begin packets as they don't initiate requests. Replies
  are never Begin packets. Error always implies End.

  Request IDs are obtained by using setIsNewRequest().

  At the end of the class there is a public section for expert usage.

  */

mExpClass(Network) RequestPacket : public ReferencedObject
{ mODTextTranslationClass(RequestPacket)
public:
			RequestPacket(od_int32 payloadsize=0);
			RequestPacket(const RequestPacket&);

    od_int32		requestID() const;
    bool		isNewRequest() const { return subID()==cBeginSubID(); }
    bool		isRequestEnd() const { return subID()<cMoreSubID(); }
    bool		isError() const      { return subID()==cErrorSubID(); }
    const void*		payload() const;

    void		getStringPayload(BufferString&) const;
    uiRetVal		getPayload(OD::JSON::Object&) const;
    uiRetVal		getPayload(IOPar&) const;
    template <class T> ArrayND<T>*	getPayload(uiRetVal&) const;
    PtrMan<PacketInterpreter>	getPayload(ObjectSet<ArrayNDInfo>&,
				   TypeSet<OD::DataRepType>&) const;

    od_int32		payloadSize() const;
    od_int32		totalSize() const
			{ return payloadSize() + mRequestPacketHeaderSize; }

    int			setIsNewRequest();   //!< conveniently returns reqID()
    void		setRequestID(od_int32); //!< for multi-packet requests
    void		setIsError()	     { setSubID( cErrorSubID() ); }
    void		setIsLast()	     { setSubID( cEndSubID() ); }

    void*		allocPayload(od_int32 size);
    void		setPayload(void*,od_int32 size); //!< buf becomes mine
    void		setStringPayload(const char*);
    bool		setPayload(const OD::JSON::Object&);
    bool		setPayload(const IOPar&);
    template <class T> bool	setPayload(const ArrayND<T>&);
    PtrMan<PacketFiller>	setPayload(const ObjectSet<ArrayNDInfo>&,
				   const TypeSet<OD::DataRepType>&);

    void		addErrMsg(BufferString&) const;

			// essentially, there is no limit unless user sets it
    static od_int32	systemSizeLimit();		//< if < 1 no limit
    static void		setSystemSizeLimit(od_int32);	//< no limit set to 0

protected:
			~RequestPacket();

    union Header
    {
			Header()
			{ int32s_[0] = 0; int32s_[1] = 0; int16s_[4] = 0; }

	od_int32	int32s_[2];
	od_int16	int16s_[5]; //!< only int16s_[4] is used
    };


    Header		header_;
    char*		payload_ = nullptr;

    static od_int16	cBeginSubID()		{ return -1; }
    static od_int16	cMoreSubID()		{ return -2; }
    static od_int16	cEndSubID()		{ return -4; }
    static od_int16	cErrorSubID()		{ return -8; }

private:

    friend class	PacketFiller;
    friend class	PacketInterpreter;

    static OD::JSON::Object	getDefaultJsonHeader(bool fortxt,od_int32 sz);
    PacketFiller*	finalize(const OD::JSON::Object&);
    PacketInterpreter*	readJsonHeader(OD::JSON::Object&,uiRetVal&) const;

public:

    bool		isOK() const;
			//!< checks whether the header is reasonable
    od_int16		subID() const;

    void		setSubID(od_int16);

    static od_int32	headerSize() { return mRequestPacketHeaderSize; }
    static od_int32	getPayloadSize(const void*);

    void*		payload(bool takeover=false);
			    /*!< takeover: delete char[] */

    void*		getRawHeader()		{ return header_.int32s_; }
    const void*		getRawHeader() const	{ return header_.int32s_; }
};


/*\brief fills a packet's payload.

  Use the sizeFor() functions to calculate the total payload size.

*/

mExpClass(Network) PacketFiller
{
public:
			PacketFiller( RequestPacket& p, int startpos=0 )
			    : pkt_(p)
			    , curpos_(startpos) {}

#   define		mNPFSizeFor(v) Network::PacketFiller::sizeFor(v)
#   define		mNPFSizeForArr(a,s) Network::PacketFiller::sizeFor(a,s)
    template <class T>
    static int		sizeFor(const T&);
    template <class T>
    static int		sizeFor(const T*,int nrelems,bool rawmode=false);
    static int		sizeFor(const char*);

    template <class T> const PacketFiller&
			put(const T&) const;
    template <class T> const PacketFiller&
			put(const T*,int nrelems,bool rawmode=false) const;
    template <class T> const PacketFiller&
			put(const ArrayND<T>&,bool rawmode=false) const;

    const PacketFiller& put(const char*) const;
    const PacketFiller&	putBytes(const void*,int nrbytes) const;

protected:

    RequestPacket&	pkt_;
    mutable int		curpos_;

};


/*\brief interprets what is in a packet.

  You need to know what should be in there. Arrays/sets are always
  preceeded by their size.

*/

mExpClass(Network) PacketInterpreter
{
public:
				PacketInterpreter( const RequestPacket& p,
						 int startpos=0 )
				    : pkt_(p)
				    , curpos_(startpos) {}

    template <class T> void	get(T&) const;
    inline bool			getBool() const;
    inline int			getInt() const;
    inline float		getFloat() const;
    inline double		getDouble() const;
    inline BufferString		getString() const;

    template <class T> void	getArr(T*,int maxsz,bool rawmode=false) const;
    template <class T>	void	getArr(ArrayND<T>&,bool rawmode=false) const;
    inline void			getSet(BufferStringSet&,int maxsz=-1) const;
    template <class T> void	getSet(TypeSet<T>&,int maxsz=-1,
					bool rawmode=false) const;

    inline void			move( int nrb ) const	{ curpos_ += nrb; }
    inline void			moveTo( int pos ) const { curpos_ = pos; }
    template <class T> void	peek(T&) const;
    inline int			peekInt() const;
    void			getBytes(void*,int nrbytes) const;

    inline bool			atEndOfPkt() const;

protected:

    const RequestPacket&	pkt_;
    mutable int			curpos_;

};



template <class T> inline
bool RequestPacket::setPayload( const ArrayND<T>& arr )
{
    ObjectSet<ArrayNDInfo> infos;
    TypeSet<OD::DataRepType> types;
    infos.add( (ArrayNDInfo*)&arr.info() );
    types += OD::GetDataRepType<T>();
    PtrMan<PacketFiller> filler = setPayload( infos, types );
    if ( !filler )
	return false;

    filler->put( arr, true );
    return true;
}


template <class T> inline
ArrayND<T>* RequestPacket::getPayload( uiRetVal& uirv ) const
{
    ObjectSet<ArrayNDInfo> infos;
    TypeSet<OD::DataRepType> types;
    PtrMan<PacketInterpreter> interp = getPayload( infos, types );
    if ( !interp || infos.isEmpty() )
	{ deepErase( infos ); return nullptr; }

    ArrayND<T>* ret = (ArrayND<T>*)getArrayND( *infos.first(), types[0] );
    deepErase( infos );
    if ( !ret || !ret->isOK() )
	{ delete ret; return nullptr; }

    interp->getArr( *ret, true );

    return ret;
}



template <class T>
inline int PacketFiller::sizeFor( const T& var )
{
    return sizeof(T);
}

template <class T>
inline int PacketFiller::sizeFor( const T* arr, int nrelems, bool rawmode )
{
    return (rawmode ? 0 : sizeof(int)) + nrelems * sizeof(T);
}

template <>
inline int PacketFiller::sizeFor( const bool& var )
{
    return sizeFor( ((int)var) );
}

template <>
inline int PacketFiller::sizeFor( const OD::String& str )
{
    return sizeof(int) + str.size();
}

template <>
inline int PacketFiller::sizeFor( const BufferString& str )
{ return sizeFor( (const OD::String&) str ); }

template <>
inline int PacketFiller::sizeFor( const StringView& str )
{ return sizeFor( (const OD::String&) str ); }

template <>
inline int PacketFiller::sizeFor( const BufferStringSet& bss )
{
    int ret = sizeof(int);
    for ( int idx=0; idx<bss.size(); idx++ )
	ret += sizeFor( bss.get(idx) );
    return ret;
}

inline int PacketFiller::sizeFor( const char* str )
{
    return sizeFor( StringView(str) );
}


template <class T>
inline const PacketFiller& PacketFiller::put( const T& var ) const
{
    return putBytes( &var, sizeof(T) );
}


template <class T>
inline const PacketFiller& PacketFiller::put( const T* arr, int nrelems,
						bool rawmode ) const
{
    if ( !rawmode )
	put( nrelems );
    return putBytes( arr, nrelems * sizeof(T) );
}

template <class T>
inline const PacketFiller& PacketFiller::put( const ArrayND<T>& arr,
					      bool rawmode ) const
{
    if ( arr.getData() )
	return put( arr.getData(), arr.totalSize(), rawmode );

    if ( !rawmode )
	put( arr.totalSize() );

    ArrayNDIter iter( arr.info() );
    do
    {
	const T val = arr.getND( iter.getPos() );
	put( val );
    } while( iter.next() );

    return *this;
}


template <>
inline const PacketFiller& PacketFiller::put( const bool& var ) const
{
    const int toput = var ? 1 : 0;
    return put( toput );
}

template <>
inline const PacketFiller& PacketFiller::put( const OD::String& str ) const
{
    const int sz = str.size();
    put( sz );
    return putBytes( str.str(), sz );
}

template <>
inline const PacketFiller& PacketFiller::put( const BufferString& str ) const
{ return put( (const OD::String&) str ); }

template <>
inline const PacketFiller& PacketFiller::put( const StringView& str ) const
{ return put( (const OD::String&) str ); }

template <>
inline const PacketFiller& PacketFiller::put( const BufferStringSet& bss ) const
{
    const int sz = bss.size();
    put( sz );
    for ( int idx=0; idx<sz; idx++ )
	put( bss.get(idx) );
    return *this;
}

inline const PacketFiller& PacketFiller::put( const char* str ) const
{ return put( StringView(str) ); }

inline const PacketFiller& PacketFiller::putBytes( const void* ptr,
						   int sz ) const
{
    if ( sz > 0 )
    {
	OD::memCopy( pkt_.payload_+curpos_, ptr, sz );
	curpos_ += sz;
    }
    return *this;
}




template <class T>
inline void PacketInterpreter::peek( T& var ) const
{
    OD::memCopy( &var, pkt_.payload_+curpos_, sizeof(T) );
}

template <class T>
inline void PacketInterpreter::get( T& var ) const
{
    getBytes( &var, sizeof(T) );
}

template <>
inline void PacketInterpreter::get( BufferString& var ) const
{
    int sz; get( sz );
    if ( sz < 1 )
    {
	if ( sz < 0 )
	    { pErrMsg("Invalid string size"); }
	var.setEmpty();
	return;
    }
    var.setBufSize( sz+1 );
    char* cstr = var.getCStr();
    getBytes( cstr, sz );
    cstr[sz] = '\0';
}

inline int PacketInterpreter::peekInt() const
{ int ret = 0; peek(ret); return ret; }
inline bool PacketInterpreter::getBool() const
{ int ret = 0; get(ret); return ret != 0; }
inline int PacketInterpreter::getInt() const
{ int ret = 0; get(ret); return ret; }
inline float PacketInterpreter::getFloat() const
{ float ret = 0.f; get(ret); return ret; }
inline double PacketInterpreter::getDouble() const
{ double ret = 0.; get(ret); return ret; }
inline BufferString PacketInterpreter::getString() const
{ BufferString ret; get(ret); return ret; }

template <class T>
inline void PacketInterpreter::getArr( T* arr, int maxsz, bool rawmode ) const
{
    int arrsz = maxsz;
    if ( !rawmode )
	get( arrsz );
    int sz = arrsz;
    if ( sz > maxsz )
	sz = maxsz;
    if ( arrsz < 1 )
	return;

    getBytes( arr, sz*sizeof(T) );
}

template <class T>
inline void PacketInterpreter::getArr( ArrayND<T>& arr, bool rawmode ) const
{
    if ( arr.getData() )
    {
	getArr( arr.getData(), arr.totalSize(), rawmode );
	return;
    }

    int arrsz = mCast(int,arr.totalSize());
    if ( !rawmode )
    {
	get( arrsz );
	if ( arrsz != arr.totalSize() )
	    return;
    }

    ArrayNDIter iter( arr.info() );
    T val;
    do
    {
	get( val );
	arr.setND( iter.getPos(), val );
    } while( iter.next() );
}

template <class T>
inline void PacketInterpreter::getSet( TypeSet<T>& ts, int maxsz,
					bool rawmode ) const
{
    int arrsz = maxsz;
    if ( !rawmode )
	get( arrsz );
    int sz = arrsz;
    if ( maxsz >= 0 && sz > maxsz )
	sz = maxsz;
    if ( arrsz < 1 )
	{ ts.setEmpty(); return; }

    ts.setSize( sz );
    getBytes( ts.arr(), sz*sizeof(T) );
}

inline void PacketInterpreter::getSet( BufferStringSet& bss,
						int maxsz ) const
{
    int setsz; get( setsz );
    int sz = setsz;
    if ( maxsz >= 0 && sz > maxsz )
	sz = maxsz;
    bss.setEmpty();
    if ( setsz < 1 )
	return;

    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString* bs = new BufferString;
	get( *bs );
	bss += bs;
    }

    for ( int idx=sz; idx<setsz; idx++ )
	getString();
}

inline void PacketInterpreter::getBytes( void* ptr, int nrbytes ) const
{
    if ( nrbytes > 0 )
    {
	OD::memCopy( ptr, pkt_.payload_+curpos_, nrbytes );
	curpos_ += nrbytes;
    }
}

inline bool PacketInterpreter::atEndOfPkt() const
{ return curpos_ >= pkt_.payloadSize(); }

} // namespace Network
