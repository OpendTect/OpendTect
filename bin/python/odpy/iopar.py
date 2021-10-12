"""
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
AUTHOR   : Bert
DATE     : August 2018

IOPar tools
###########

"""

import os

def read_line( words ):
  """Interprets one line from an IOPar (ascii) file

  Parameters:
    * words (string): One line as read from an open text file

  Returns:
    * tuple of two key,val strings interpreted from the input line
  Everything right of the first colon is part of the value string
  val can be an empty string

  """
  words = words.split(':')
  cleanwords = [x.strip() for x in words]
  nrwords = len(cleanwords)
  ret = list()
  if nrwords > 0:
    ret.append( cleanwords[0] )
  if nrwords > 1:
    ret.append( ':'.join(cleanwords[1:nrwords]) )
  else:
    ret.append( ':'.join('') )
  return ret

def read_from_iopar( fnm, searchkey ):
  """Return the value of a key from an OpendTect IOPar text file

  Parameters:
    * fnm (string): Full path to an OpendTect IOPar (ascii) file
    * searchkey (string): Key in the IOPar for which the value string
      is requested
  
  Returns:
    * str: If the searchkey is present in the file, value string the corresponds
  to it. Can be an empty string. None or empty string otherwise.

  """
  if not os.path.isfile(fnm):
    return None

  with open( fnm, 'r' ) as fp:
    for line in fp:
      key,val = read_line(line)
      if key == searchkey:
        return val

  return None
