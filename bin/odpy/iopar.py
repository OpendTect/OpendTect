#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Bert
# DATE     : August 2018
#
# IOPar tools
#

def read_line( fp, bin ):
  if bin:
    words = fp.readline().decode('utf8').split(":")
  else:
    words = fp.readline().split(":")
  return [x.strip() for x in words]
