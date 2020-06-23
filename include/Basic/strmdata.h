#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		3-4-1996
 Contents:	Data on any stream
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

    void mDeprecated	transferTo(StreamData&);	//!< retains file name

    void		close();
    bool		usable() const;

    void		setFileName( const char* fn );
    const char*		fileName() const;

    std::ios*		streamPtr() const;

    std::istream*	iStrm() const { return impl_->istrm_; }
    std::ostream*	oStrm() const { return impl_->ostrm_; }

    void                setIStrm( std::istream* );
    void                setOStrm( std::ostream* );

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
