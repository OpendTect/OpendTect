#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "ptrman.h"
#include "bufstring.h"
#include <iosfwd>


/*!\brief Holds data to use and close an iostream. Usually created by
  StreamProvider. */


mExpClass(Basic) StreamData
{
public:

			StreamData();
			StreamData(const StreamData&)	= delete;
			StreamData(StreamData&&);

    StreamData&		operator=(const StreamData&)	= delete;
    StreamData&		operator=(StreamData&&);

    mDeprecatedObs void transferTo(StreamData&);	//!< retains file name

    void		close();
    bool		usable() const;

    void		setFileName( const char* fn );
    const char*		fileName() const;

    std::ios*		streamPtr() const;

    std::istream*	iStrm() const { return impl_->istrm_; }
    std::ostream*	oStrm() const { return impl_->ostrm_; }

    void		setIStrm( std::istream* );
    void		setOStrm( std::ostream* );

    //Internal use (unless you're making connectors to weird external streams)
    mExpClass(Basic) StreamDataImpl
    {
    public:
	virtual void	close();
	virtual		~StreamDataImpl() {}
	BufferString	fname_;
	std::istream*	istrm_ = nullptr;
	std::ostream*	ostrm_ = nullptr;
    };

    void setImpl(StreamDataImpl*);

private:

    PtrMan<StreamDataImpl>	impl_;

};
