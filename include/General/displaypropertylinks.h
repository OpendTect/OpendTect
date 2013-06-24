#ifndef displaypropertylinks_h
#define displaypropertylinks_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl & N. Hemstra
 Date:		September 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "factory.h"
#include "namedobj.h"
#include "threadlock.h"

class IOPar;
class DisplayPropertyHolder;
class DisplayPropertyLink;

/*!brief Manages property links between differerent displays.

   Property links can be ranges, color sequences and other parameters.

   The intelligence on what can be linked lies in the subclasses of
   DisplayPropertyLink.

   Displays that are able to link properties to must inherit
   DisplayPropertyHolder and in addition must be added to the
   DisplayLinkManager.

   To create a link from a certain DisplayPropertyHolder,
   DisplayLinkManager::createPossibleLinks should be called. This fills an
   ObjectSet<DisplayPropertyLink>&, and one or more links in that set
   may be enabled by calling DisplayPropertyHolder::addDisplayPropertyLink().

   The link is removed by calling
   DisplayPropertyHolder::removeDisplayPropertyLink().

*/



mExpClass(General) DisplayLinkManager
{
public:
    				DisplayLinkManager();
    				~DisplayLinkManager();
    static DisplayLinkManager&	getImpl();

    int				nrHolders() const;
    int				addHolder(DisplayPropertyHolder*);
    const DisplayPropertyHolder* getHolder(int) const;

    void			createPossibleLinks(DisplayPropertyHolder*,
					ObjectSet<DisplayPropertyLink>&);
    				//!<DisplayPropertyLinks become yours

    int				nrDisplayPropertyLinks() const;
    int				getDisplayPropertyLinkID(int idx) const;
    int				addDisplayPropertyLink(DisplayPropertyLink*);
    				/*!<PropertyLink becomes mine, returns id for
				    new link. */
    void			removeDisplayPropertyLink(int id);
    DisplayPropertyLink*	getDisplayPropertyLink(int id);
    const DisplayPropertyLink*	getDisplayPropertyLink(int id) const;

    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

protected:

    friend			class DisplayPropertyHolder;
    void			removeHolder(DisplayPropertyHolder*);

    ObjectSet<DisplayPropertyHolder> holders_;
    int				freeholderid_;
    mutable Threads::Lock	lock_;

    ObjectSet<DisplayPropertyLink> propertylinks_;
    TypeSet<int>		propertylinkids_;
    int				freelinkid_;
};





mExpClass(General) DisplayPropertyHolder
{
public:
			DisplayPropertyHolder(bool reg);

    virtual		~DisplayPropertyHolder();

    int			propertyHolderID() const;
    virtual const char*	getDisplayPropertyHolderName() const;

private:
    friend		class DisplayLinkManager;
    int			propertyholderid_;

};


mExpClass(General) DisplayPropertyLink : public NamedObject
{
public:
    mDefineFactory1ParamInClass(DisplayPropertyLink,
	    ObjectSet<DisplayPropertyHolder>&,factory);

    virtual const char*		type() const				= 0;
    virtual const char*		userType() const;

    virtual bool		init();
    				//Called when added to manager, before
				//this call, no callbacks should be set.
				
    bool			addHolder(DisplayPropertyHolder*);
    void			removeHolder(DisplayPropertyHolder*);
    bool			isValid() const;
    virtual void		getDescription(BufferString&) const;

    int				nrDisplayPropertyHolders() const;
    const DisplayPropertyHolder* getDisplayPropertyHolder(int idx) const;

protected:
    friend			class DisplayLinkManager;
    virtual bool		syncNow(const DisplayPropertyHolder* origin,
	    				DisplayPropertyHolder* target);
    				//If Target is zero, sync all but origin

    virtual bool		isHolderOK(const DisplayPropertyHolder*);
    				DisplayPropertyLink(DisplayPropertyHolder*,
						    DisplayPropertyHolder*);

    ObjectSet<DisplayPropertyHolder>	holders_;
};

#endif

