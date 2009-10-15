//===========================================================================
//                                                                          =
//   This program is free software; you can redistribute it and/or modify   =
//   it under the terms of the GNU General Public License as published by   =
//   the Free Software Foundation; either version 2 of the License, or      =
//   (at your option) any later version.                                    =
// ------------------------------------------------------------------------ =
//                  TTTTT    OOO    PPPP    EEEE    DDDD                    =
//                  T T T   O   O   P   P   E       D   D                   =
//                    T    O     O  PPPP    EEE     D    D                  =
//                    T     O   O   P       E       D   D                   =
//                    T      OOO    P       EEEEE   DDDD                    =
//                                                                          =
//   This file is a part of Toped project (C) 2001-2007 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Sep 14 1999
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: GDSII parser
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include <cmath>
#include <sstream>
#include <string>
#include <time.h>
#include "gds_io.h"
#include "outbox.h"
#include "../tpd_DB/tedat.h"
#include "../tpd_DB/tedesign.h"

//==============================================================================
GDSin::GdsRecord::GdsRecord()
{
   _record = DEBUG_NEW byte[0xffff];
   _valid = false;
   _recLen = 0;
   _recType = gds_HEADER;
   _dataType = gdsDT_NODATA;
   _numread = 0;
   _index = 0;
}

void GDSin::GdsRecord::getNextRecord(wxFFile& Gf, word rl, byte rt, byte dt)
{
   _recLen = rl; _recType = rt; _dataType = dt;
   if (rl)
   {
      _numread = Gf.Read(_record, _recLen);
      _valid = (_numread == _recLen) ? true : false;
   }
   else
   {
      _numread = 0;
      _valid = true;
   }
}

GDSin::GdsRecord::GdsRecord(byte rt, byte dt, word rl)
{
   _recType = rt; _dataType = dt;
   _recLen = rl+4; _index = 0;
   // compensation for odd length ASCII string
   if ((gdsDT_ASCII == _dataType) && (rl % 2)) _recLen++;
   _record = DEBUG_NEW byte[_recLen];
   add_int2b(_recLen);
   _record[_index++] = _recType;
   _record[_index++] = _dataType;
}

size_t GDSin::GdsRecord::flush(wxFFile& Gf)
{
   assert(_index == _recLen);
   size_t bytes_written = Gf.Write(_record, _recLen);
   /*@TODO !!! Error correction HERE instead of assertetion */
   assert(bytes_written == _recLen);
   return bytes_written;
}

bool GDSin::GdsRecord::retData(void* var, word curnum, byte len) const
{
   byte      *rlb;
   char      *rlc;
   switch (_dataType)
   {
      case gdsDT_NODATA:// no data present
         var = NULL;return false;
      case gdsDT_BIT:// bit array
         rlb = (byte*)var; //assign pointer
         switch (len)
         {
            case 32:
               rlb[3] = _record[0];
               rlb[2] = _record[1];
               rlb[1] = _record[2];
               rlb[0] = _record[3];
               break;
            case 16:
               rlb[1] = _record[0];
               rlb[0] = _record[1];
               break;
            case 8:
               rlb[0] = _record[0];
               break;
            default:
               var = NULL;break;
         }
         break;
      case gdsDT_INT2B://2-byte signed integer
         rlb = (byte*)var;   //assign pointer
         rlb[0] = _record[curnum+1];
         rlb[1] = _record[curnum+0];
         break;
      case gdsDT_INT4B: // 4-byte signed integer
         rlb = (byte*)var;//assign pointer
         rlb[0] = _record[curnum+3];
         rlb[1] = _record[curnum+2];
         rlb[2] = _record[curnum+1];
         rlb[3] = _record[curnum+0];
         break;
      case gdsDT_REAL4B:// 4-bit real
      {
         // WARNING!!! not used and never checked !!!!
         _sg_int8 sign = (0x80 & _record[curnum])? -1:1; //sign
         byte exponent = 0x7f & _record[curnum]; // exponent
         _sg_int32 mantissa = 0; // mantissa
         byte* mant = (byte*)&mantissa;// take the memory possition
         mant[3] = 0x0;
         for (int i = 0; i < 3; i++)
            mant[i] = _record[curnum+3-i];
         double *rld = (double*)var; // assign pointer
         *rld = sign*(mantissa/pow(2.0,24)) * pow(16.0, exponent-64);
         break;
      }
      case gdsDT_REAL8B:// 8-byte real
         *(double*)var = gds2ieee(&(_record[curnum]));
         break;
      case gdsDT_ASCII:// String
         if (len > 0)
         {
            rlc = DEBUG_NEW char[len+1];
            memcpy(rlc, &(_record[curnum*len]), len);
            rlc[len] = 0x0;
         }
         else
         {
            rlc = DEBUG_NEW char[_recLen+1];
            memcpy(rlc, _record, _recLen);
            rlc[_recLen] = 0x0;
         }
         *((std::string*)var) = rlc;
         delete [] rlc;
         break;
   }
   return true;
}

double GDSin::GdsRecord::gds2ieee(byte* gds) const
{
   // zero is an exception (as always!) so check it first
   byte zerocheck;
   for (zerocheck = 1; zerocheck < 8; zerocheck++)
      if (0x00 != gds[zerocheck]) break;
   if (8 == zerocheck) return 0;
   // adjusting the exponent
   byte expcw [2] = {gds[1],gds[0]};
   word& expc = *((word*)&expcw);
   // IEEE has 2x exponent, while GDSII has 16x. To compensate
   // we need to multiply the exponent by 4, but, IEEE has 4 bits
   // wider exponent, so we need to shift GDS exp right by 4 positons.
   // in result, we are shifting right by two positions
   expc >>= 2;
   // fill-up the two leftmost positions of the exponent with
   // bits opposite to the excess bit
   // and also copy the excess bit
   if (!(0x40 & gds[0])){expc |= 0x3000; expc &= 0xBFFF;}
   else                 {expc &= 0xCFFF; expc |= 0x4000;}
   // clean-up
   expc &= 0x7FC0; // clean-up
   // compensate the difference in the excess notation
   expc -= 0x10;
   // Now normalize the mantissa - shift left until first 1 drops-out
   // The last byte will get some rubbish in it's LSBits, but it 
   // shouldn't matter because last four buts of the mantissa will be
   // chopped-out
   byte carry;
   do {
      carry = gds[1] & 0x80;
      for (byte i = 1; i < 7; i++) {
         gds[i] <<= 1;
         gds[i] |= (gds[i+1] >> 7); //carry
      }
      expc -= 0x10;
   } while (0 == carry);
   // copy the sign bit
   if    (0x80 & gds[0]) expc |= 0x8000;
   else                  expc &= 0x7FFF;
   // transfer the result into a new 8 byte string ...
   // ... copy the exponent first
   byte ieee[8];
   ieee[7] = expcw[1];ieee[6] = expcw[0];
   //... then copy the mantissa
   for  (byte i = 1; i <7; i++ ) {
      ieee[6-i] = (gds[i] << 4) | (gds[i+1] >> 4);
   }
   // last nibble of the mantissa
   ieee[6] |= gds[1] >> 4;
   // that should be it !
   return *((double*)&ieee);
}

byte* GDSin::GdsRecord::ieee2gds(double inval)
{
   byte* ieee = ((byte*)&inval);
   byte* gds = DEBUG_NEW byte[8];
   // zero is an exception (as always!) so check it first
   if (0 == inval) {
      for (byte i = 0; i < 8; gds[i++] = 0x00);
      return gds;
   }
   //copy the mantissa
   for  (byte i = 1; i < 7; i++ ) {
      gds[i] = (ieee[7-i] << 4) | (ieee[6-i] >> 4);
   }
   gds[7] = ieee[0] << 4;
   // adjusting the exponent
   byte expcw [2] = {ieee[6],ieee[7]};
   word& expc = *((word*)&expcw);
   expc &= 0x7FF0; // clean-up
   //compensate the difference in excess notations
   expc += 0x10;
   // Now normalize the mantissa - shift right until the two LSBit of
   // the exponent are 00. First shift should introduce 1 on the leftmost
   // position of the manissa to take in mind the explicit 1 in the ieee
   // notation
   gds[0] = 0x01; 
   do {
      for (byte i = 7; i > 0; i--) {
         gds[i] >>= 1;
         gds[i] |= (gds[i-1] << 7); //carry
      }
      gds[0] = 0x00; 
      expc += 0x10;
   } while (0 != (expc & 0x0030));
   //make sure we are not trying to convert a number bigger than the one 
   //that GDS notation can cope with
   // copy the excess bit
   if (!(0x4000 & expc)) expc &= 0xEFFF;
   else                  expc |= 0x1000;
   // now multiply the exponent by 4 to convert in the 16x GDS exponent
   // here we are loosing silently the two most significant bits from the
   // ieee exponent. 
   expc <<= 2;
   // copy the sign bit
   if   (0x80 & ieee[7])  expc |= 0x8000;
   else                   expc &= 0x7FFF;
   gds[0] = expcw[1];
   return gds;
}

void GDSin::GdsRecord::add_int2b(const word data)
{
   byte* recpointer = (byte*)&data;
   _record[_index++] = recpointer[1];
   _record[_index++] = recpointer[0];
}

void GDSin::GdsRecord::add_int4b(const int4b data)
{
   byte* recpointer = (byte*)&data;
   _record[_index++] = recpointer[3];
   _record[_index++] = recpointer[2];
   _record[_index++] = recpointer[1];
   _record[_index++] = recpointer[0];
}

void GDSin::GdsRecord::add_ascii(const char* data)
{
   word slen = strlen(data);
   bool compensate = (0 != (slen % 2));
   word strindex = 0;
   while (strindex < slen)
      _record[_index++] = data[strindex++];
   if (compensate) _record[_index++] = 0x00;
   assert(compensate ? ((_recLen-4) == slen+1) : ((_recLen-4) == slen) );
}

void GDSin::GdsRecord::add_real8b(const real data)
{
   byte* gdsreal = ieee2gds(data);
   for (byte i = 0; i < 8; i++) _record[_index++] = gdsreal[i];
   delete [] gdsreal;
}

GDSin::GdsRecord::~GdsRecord()
{
   delete[] _record;
}

//==============================================================================
// class GdsInFile
//==============================================================================
GDSin::GdsInFile::GdsInFile(std::string fn)
{
   _hierTree = NULL;
   _gdsiiWarnings = 0;
   _fileName = fn;
   _filePos = 0;
   _prgrs_pos = 0;
   _library = NULL;
   _cRecord = DEBUG_NEW GdsRecord();
   tell_log(console::MT_INFO, std::string("GDSII input file: \"") + fn + std::string("\""));
   wxString wxfname(_fileName.c_str(), wxConvUTF8 );
   _gdsFh.Open(wxfname.c_str(),wxT("rb"));
   std::ostringstream info;
   if (!(_gdsFh.IsOpened()))
   {// open the input file
      info << "File "<< _fileName <<" can NOT be opened";
      tell_log(console::MT_ERROR,info.str());
      return;
   }
   wxFileOffset _fileLength = _gdsFh.Length();
   // The size of GDSII files is originaly multiple by 2048. This is
   // coming from the acient years when this format was supposed to be written 
   // on the magnetic tapes. In order to keep the tradition it's a good idea 
   // to check the file size and to issue a warning if it is not multiple on 2048.
//   div_t divi = div(file_length,2048);
//   if (divi.rem != 0) AddLog('W',"File size is not multiple of 2048");
   TpdPost::toped_status(console::TSTS_PRGRSBARON, _fileLength);

   do
   {// start reading
      if (getNextRecord())
      {
         switch (_cRecord->recType())
         {
            case gds_HEADER:      _cRecord->retData(&_streamVersion);
               info.clear();
               info << "Stream version: " << _streamVersion;
               tell_log(console::MT_INFO, info.str());
               break;
            case gds_BGNLIB:      getTimes(_cRecord);
               break;
            case gds_LIBDIRSIZE:   _cRecord->retData(&_libDirSize);
               break;
            case gds_SRFNAME:      _cRecord->retData(&_srfName);
               break;
            case gds_LIBSECUR:// I don't need this info. Does anybody need it?
               break;
            case gds_LIBNAME: {  // down in the hierarchy.
               std::string libname;
               _cRecord->retData(&libname);
               //Start reading the library structure
              _library = DEBUG_NEW GdsLibrary(this, libname);
               //build the hierarchy tree
               _library->linkReferences(this);
               closeFile();// close the input stream
               TpdPost::toped_status(console::TSTS_PRGRSBAROFF);
               tell_log(console::MT_INFO, "Done");
               return; // go out
            }
            default:   //parse error - not expected record type
               throw EXPTNreadGDS("GDS header - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

bool GDSin::GdsInFile::reopenFile()
{
   _gdsiiWarnings = 0;
   _filePos = 0;
   _prgrs_pos = 0;
   _cRecord = DEBUG_NEW GdsRecord();
   wxString wxfname(_fileName.c_str(), wxConvUTF8 );
   _gdsFh.Open(wxfname.c_str(),wxT("rb"));
   if (!(_gdsFh.IsOpened()))
   {// open the input file
      std::ostringstream info;
      info << "File "<< _fileName <<" can NOT be reopened";
      tell_log(console::MT_ERROR,info.str());
      return false;
   }
   return true;
}

void GDSin::GdsInFile::closeFile()
{
   if (_gdsFh.IsOpened())
      _gdsFh.Close();
   if (NULL != _cRecord)
      delete _cRecord;
}

GDSin::GdsStructure* GDSin::GdsInFile::getStructure(const std::string nm)
{
   return _library->getStructure(nm);
}

void GDSin::GdsInFile::collectLayers(GdsLayers& lays)
{
   _library->collectLayers(lays);
}

std::string GDSin::GdsInFile::libname() const
{
   return _library->libName();
}

void GDSin::GdsInFile::hierOut()
{
   _hierTree = _library->hierOut();
}

void GDSin::GdsInFile::setPosition(wxFileOffset filePos)
{
   _gdsFh.Seek(filePos, wxFromStart);
}

void GDSin::GdsInFile::getTimes(GdsRecord *wr)
{
   tm tmodif_bt;
   tm taccess_bt;
   word cw;
   for (int i = 0; i<wr->recLen()/2; i++)
   {
      wr->retData(&cw,2*i);
      switch (i)
      {
         case 0 :tmodif_bt.tm_year   = cw - 1900;break;
         case 1 :tmodif_bt.tm_mon    = cw - 1;break;
         case 2 :tmodif_bt.tm_mday   = cw;break;
         case 3 :tmodif_bt.tm_hour   = cw;break;
         case 4 :tmodif_bt.tm_min    = cw;break;
         case 5 :tmodif_bt.tm_sec    = cw;break;
         case 6 :taccess_bt.tm_year  = cw - 1900;break;
         case 7 :taccess_bt.tm_mon   = cw - 1;break;
         case 8 :taccess_bt.tm_mday  = cw;break;
         case 9 :taccess_bt.tm_hour  = cw;break;
         case 10:taccess_bt.tm_min   = cw;break;
         case 11:taccess_bt.tm_sec   = cw;break;
      }
   }
   _tModif  = TpdTime(tmodif_bt );
   _tAccess = TpdTime(taccess_bt);
   std::stringstream info;
   if (_tModif.status())
   {
      info << "Library was last modified: " << _tModif();
      tell_log(console::MT_INFO, info.str());
   }
   else
   {
      info << "Library modification time stamp is invalid";
      tell_log(console::MT_WARNING, info.str());
   }
   info.str("");
   if (_tAccess.status())
   {
      info << "Library was last accessed: " << _tAccess();
      tell_log(console::MT_INFO, info.str());
   }
   else
   {
      info << "Library access time stamp is invalid";
      tell_log(console::MT_WARNING, info.str());
   }
}

bool GDSin::GdsInFile::getNextRecord()
{
   char recheader[4]; // record header
   size_t numread = _gdsFh.Read(&recheader,4);// read record header
   if (numread != 4)
      return NULL;// error during read in
   char rl[2];
   rl[0] = recheader[1];
   rl[1] = recheader[0];
   word reclen = *(word*)rl - 4; // record lenght
//   GdsRecord* retrec = DEBUG_NEW GdsRecord(_gdsFh, reclen, recheader[2],recheader[3]);
   _cRecord->getNextRecord(_gdsFh, reclen, recheader[2],recheader[3]);
   _filePos += reclen + 4;    // update file position
   if (2048 < (_filePos - _prgrs_pos))
   {
      _prgrs_pos = _filePos;
      TpdPost::toped_status(console::TSTS_PROGRESS, _prgrs_pos);
   }
   if (_cRecord->valid()) return true;
   else return false;// error during read in
}

double GDSin::GdsInFile::libUnits()
{
   return _library->dbu()/_library->uu();
}

double GDSin::GdsInFile::userUnits()
{
   return _library->uu();
}

GDSin::GdsInFile::~GdsInFile()
{
   delete _library;
   // get rid of the hierarchy tree
   const GDSHierTree* var1 = _hierTree;
   while (var1)
   {
      const GDSHierTree* var2 = var1->GetLast();
      delete var1; var1 = var2;
   }
}

//==============================================================================
// class GdsLibrary
//==============================================================================
GDSin::GdsLibrary::GdsLibrary(GdsInFile* cf, std::string libName)
{
   _libName = libName;
   _maxver = 3;
   GdsStructure* cstr = NULL;
   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_FORMAT:// skipped record !!!
               tell_log(console::MT_WARNING, " GDSII record type 'FORMAT' skipped");
               cf->incGdsiiWarnings();
               break;
            case gds_MASK:// skipped record !!!
               tell_log(console::MT_WARNING, " GDSII record type 'MASK' skipped");
               cf->incGdsiiWarnings();
               break;
            case gds_ENDMASKS:// skipped record !!!
               tell_log(console::MT_WARNING, " GDSII record type 'ENDMASKS' skipped");
               cf->incGdsiiWarnings();
               break;
            case gds_REFLIBS:// skipped record !!!
               tell_log(console::MT_WARNING, " GDSII record type 'REFLIBS' skipped");
               cf->incGdsiiWarnings();
               break;
            case gds_ATTRTABLE:// skipped record !!!
               tell_log(console::MT_WARNING, " GDSII record type 'ATTRTABLE' skipped");
               cf->incGdsiiWarnings();
               break;
            case gds_FONTS:// Read fonts
               for(byte i = 0; i < 4; i++)
                  cr->retData(&(_allFonts[i]),i,44);
               break;
            case gds_GENERATION:   cr->retData(&_maxver);
               break;
            case gds_UNITS:   
               cr->retData(&_uu,0,8); // database units in one user unit
               cr->retData(&_dbu,8,8); // database unit in meters
               break;
            case gds_BGNSTR:
               cstr = DEBUG_NEW GdsStructure(cf, cr->recLen());
               _structures[cstr->strctName()] = cstr;
               break;
            case gds_ENDLIB://end of library, exit form the procedure
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS Library - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsLibrary::linkReferences(GdsInFile* const cf)
{
   for (StructureMap::const_iterator CSTR = _structures.begin(); CSTR != _structures.end(); CSTR++)
   {//for every structure
      CSTR->second->linkReferences(cf, this);
   }
}

GDSin::GdsStructure* GDSin::GdsLibrary::getStructure(const std::string selection)
{
   StructureMap::iterator striter;
   if (_structures.end() != (striter = _structures.find(selection)))
      return striter->second;
   else
      return NULL;
}

void GDSin::GdsLibrary::collectLayers(GdsLayers& layers)
{
   for (StructureMap::const_iterator CSTR = _structures.begin(); CSTR != _structures.end(); CSTR++)
      CSTR->second->collectLayers(layers, false);
}

GDSin::GDSHierTree* GDSin::GdsLibrary::hierOut()
{
   GDSHierTree* Htree = NULL;
   for (StructureMap::const_iterator CSTR = _structures.begin(); CSTR != _structures.end(); CSTR++)
      if (!CSTR->second->haveParent()) 
         Htree = CSTR->second->hierOut(Htree, NULL);
   return Htree;
}


GDSin::GdsLibrary::~GdsLibrary()
{
   for (StructureMap::const_iterator CSTR = _structures.begin(); CSTR != _structures.end(); CSTR++)
      delete (CSTR->second);
}

//==============================================================================
// class GdsStructure
//==============================================================================
void GDSin::GdsStructure::import(GdsInFile *cf, laydata::tdtcell* dst_cell,
                                 laydata::tdtlibdir* tdt_db, const LayerMapGds& _theLayMap)
{
   std::string strctName;
   //initializing
   const GdsRecord* cr = cf->cRecord();
   cf->setPosition(_filePos);
   do
   { //start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_STRNAME:
               cr->retData(&strctName);
               assert(strctName == _strctName); // @TODO - throw an exception HERE!
               break;
            case gds_PROPATTR:
               tell_log(console::MT_WARNING, " GDSII record type 'PROPATTR' skipped");
               cf->incGdsiiWarnings();
               break;
            case gds_STRCLASS:
               tell_log(console::MT_WARNING, " GDSII record type 'STRCLASS' skipped");
               cf->incGdsiiWarnings();// CADANCE internal use only
               break;
            case gds_BOX:
               importBox(cf, dst_cell, _theLayMap);
               break;
            case gds_BOUNDARY:
               importPoly(cf, dst_cell, _theLayMap);
               break;
            case gds_PATH:
               importPath(cf, dst_cell, _theLayMap);
               break;
            case gds_TEXT:
               importText(cf, dst_cell, (*tdt_db)()->UU(), _theLayMap);
               break;
            case gds_SREF:
               importSref(cf, dst_cell, tdt_db, _theLayMap);
               break;
            case gds_AREF:
               importAref(cf, dst_cell, tdt_db, _theLayMap);
               break;
            case gds_NODE:// skipped record !!!
               tell_log(console::MT_WARNING, " GDSII record type 'NODE' skipped");
               cf->incGdsiiWarnings();
               skimNode(cf);
               break;
            case gds_ENDSTR:// end of structure, exit point
               dst_cell->resort();
               _traversed = true;
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS structure - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

GDSin::GdsStructure::GdsStructure(GdsInFile *cf, word bgnRecLength)
{
   _traversed = false;
   _filePos = cf->filePos();
   _beginRecLength = bgnRecLength + 4;
   //initializing
   _haveParent = false;
   const GdsRecord* cr = cf->cRecord();
   do
   { //start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_NODE:
               tell_log(console::MT_WARNING, " GDSII record type 'NODE' skipped");
               cf->incGdsiiWarnings();
               skimNode(cf);
               break;
            case gds_PROPATTR:
               tell_log(console::MT_WARNING, " GDSII record type 'PROPATTR' skipped");
               cf->incGdsiiWarnings();
               break;
            case gds_STRCLASS:// skipped record !!!
               tell_log(console::MT_WARNING, " GDSII record type 'STRCLASS' skipped");
               cf->incGdsiiWarnings();// CADANCE internal use only
               break;
            case gds_STRNAME:
               cr->retData(&_strctName);
               tell_log(console::MT_INFO,std::string("...") + _strctName);
               break;
            case gds_BOX      : skimBox(cf)     ;  break;
            case gds_BOUNDARY : skimBoundary(cf);  break;
            case gds_PATH     : skimPath(cf)    ;  break;
            case gds_TEXT     : skimText(cf)    ;  break;
            case gds_SREF     : skimSRef(cf)    ;  break;
            case gds_AREF     : skimARef(cf)    ;  break;
            case gds_ENDSTR   :// end of structure, exit point
               _strSize = cf->filePos() - _filePos;
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS structure - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::updateContents(int2b layer, int2b dtype)
{
   _contSummary[layer].insert(dtype);
}


void GDSin::GdsStructure::linkReferences(GdsInFile* const cf, GdsLibrary* const library)
{
   for (NameSet::const_iterator CRN = _referenceNames.begin(); CRN != _referenceNames.end(); CRN++)
   {
      GdsStructure* ws2 = library->getStructure(*CRN);
      if (ws2)
      {
         _children.push_back(ws2);
         ws2->_haveParent = true;
      }
      else
      {
         char wstr[256];
         sprintf(wstr," Structure %s is referenced, but not defined!",CRN->c_str() );
         tell_log(console::MT_WARNING,wstr);
         cf->incGdsiiWarnings();
      }
   }
}

void GDSin::GdsStructure::collectLayers(GdsLayers& layers_map, bool hier)
{
   for (GdsLayers::const_iterator CL = _contSummary.begin(); CL != _contSummary.end(); CL++)
   {
      WordSet& data_types = layers_map[CL->first];
      data_types.insert(CL->second.begin(), CL->second.end());
   }
   if (!hier) return;
   for (ChildStructure::const_iterator CSTR = _children.begin(); CSTR != _children.end(); CSTR++)
      if (NULL == (*CSTR)) continue;
   else
      (*CSTR)->collectLayers(layers_map, hier);
}

GDSin::GDSHierTree* GDSin::GdsStructure::hierOut(GDSHierTree* Htree, GdsStructure* parent)
{
   // collecting hierarchical information
   Htree = DEBUG_NEW GDSHierTree(this, parent, Htree);
   for (ChildStructure::const_iterator CSTR = _children.begin(); CSTR != _children.end(); CSTR++)
   {
      if (NULL == (*CSTR)) continue;
      else
      {
         Htree = (*CSTR)->hierOut(Htree,this);
      }
   }
   return Htree;
}

void GDSin::GdsStructure::skimBox(GdsInFile* cf)
{
   int2b layer;
   int2b singleType;

   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_LAYER    : cr->retData(&layer); break;
            case gds_BOXTYPE  : cr->retData(&singleType);break;
            case gds_ELFLAGS  :
            case gds_PLEX     :
            case gds_PROPATTR :
            case gds_PROPVALUE:
            case gds_XY       : break;
            case gds_ENDEL://end of element, exit point
               updateContents(layer, singleType);
               return;
            default: //parse error - not expected record type
               throw EXPTNreadGDS("GDS box - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::skimBoundary(GdsInFile* cf)
{
   int2b layer;
   int2b singleType;

   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_LAYER    : cr->retData(&layer);break;
            case gds_DATATYPE : cr->retData(&singleType);break;
            case gds_ELFLAGS  :
            case gds_PLEX     :
            case gds_PROPATTR :
            case gds_PROPVALUE:
            case gds_XY       : break;
            case gds_ENDEL    ://end of element, exit point
               updateContents(layer, singleType);
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS boundary - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::skimPath(GdsInFile* cf)
{
   int2b layer;
   int2b singleType;

   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_LAYER    : cr->retData(&layer); break;
            case gds_DATATYPE : cr->retData(&singleType); break;
            case gds_ELFLAGS  :
            case gds_PLEX     :
            case gds_PATHTYPE :
            case gds_WIDTH    :
            case gds_BGNEXTN  :
            case gds_ENDEXTN  :
            case gds_PROPATTR :
            case gds_PROPVALUE:
            case gds_XY       : break;
            case gds_ENDEL://end of element, exit point
               updateContents(layer, singleType);
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS path - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (cr->recType() != gds_ENDEL);
}

void GDSin::GdsStructure::skimSRef(GdsInFile* cf)
{
   std::string strctName;

   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_SNAME    : cr->retData(&strctName);break;
            case gds_ELFLAGS  :
            case gds_PLEX     :
            case gds_STRANS   :
            case gds_MAG      :
            case gds_ANGLE    :
            case gds_XY       :
            case gds_PROPATTR :
            case gds_PROPVALUE: break;
            case gds_ENDEL://end of element, exit point
               _referenceNames.insert(strctName);
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS sref - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::skimARef(GdsInFile* cf)
{
   std::string strctName;

   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_SNAME:cr->retData(&strctName); break;
            case gds_ELFLAGS  :
            case gds_PLEX     :
            case gds_STRANS   :
            case gds_MAG      :
            case gds_ANGLE    :
            case gds_XY       :
            case gds_COLROW   :
            case gds_PROPATTR :
            case gds_PROPVALUE: break;
            case gds_ENDEL://end of element, exit point
               _referenceNames.insert(strctName);
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS aref - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::skimText(GdsInFile* cf)
{
   int2b layer;
   int2b singleType;

   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_LAYER    : cr->retData(&layer); break;
            case gds_TEXTTYPE : cr->retData(&singleType); break;
            case gds_ELFLAGS  :
            case gds_PLEX     :
            case gds_PATHTYPE :
            case gds_WIDTH    :
            case gds_PROPATTR :
            case gds_PROPVALUE:
            case gds_PRESENTATION:
            case gds_STRANS   :
            case gds_MAG      :
            case gds_ANGLE    :
            case gds_XY       :
            case gds_STRING   : break;
            case gds_ENDEL://end of element, exit point
               updateContents(layer, singleType);
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS text - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::skimNode(GdsInFile* cf)
{
   int2b layer, singleType;
   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_LAYER    : cr->retData(&layer);      break;
            case gds_NODETYPE : cr->retData(&singleType); break;
            case gds_ELFLAGS  :
            case gds_PLEX     :
            case gds_XY       : break;
            case gds_PROPATTR : cf->incGdsiiWarnings(); break;
            case gds_PROPVALUE: tell_log(console::MT_WARNING, "GDS node - PROPVALUE record ignored");
               cf->incGdsiiWarnings(); break;
            case gds_ENDEL://end of element, exit point
               updateContents(layer, singleType);
               return;
            default: //parse error - not expected record type
               throw EXPTNreadGDS("GDS node - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::importBox(GdsInFile* cf, laydata::tdtcell* dst_cell, const LayerMapGds& theLayMap)
{
   int2b       layer;
   int2b       singleType;
   word        tdtlaynum;

   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_ELFLAGS://readElflags(cr);// seems that it's not used
               break;
            case gds_PLEX:   //readPlex(cr);// seems that it's not used
               break;
            case gds_LAYER: cr->retData(&layer);
               break;
            case gds_BOXTYPE:cr->retData(&singleType);
               break;
            case gds_PROPATTR:
               cf->incGdsiiWarnings(); break;
            case gds_PROPVALUE: tell_log(console::MT_WARNING, "GDS box - PROPVALUE record ignored");
               cf->incGdsiiWarnings(); break;
            case gds_XY:
               if ( theLayMap.getTdtLay(tdtlaynum, layer, singleType) )
               {
                  //one point less because fist and last point coincide
                  word numpoints = (cr->recLen())/8 - 1;
                  assert(numpoints == 4);
                  pointlist   plist;
                  plist.reserve(numpoints);
                  for(word i = 0; i < numpoints; i++)
                     plist.push_back(GDSin::get_TP(cr, i));

                  laydata::valid_poly check(plist);

                  if (!check.valid())
                  {
                     std::ostringstream ost;
                     ost << "Box check fails - {" << check.failtype()
                         << " Layer: " << layer
                         << " Data type: " << singleType
                         << " }";
                     tell_log(console::MT_ERROR, ost.str());
                  }
                  laydata::tdtlayer* dwl = static_cast<laydata::tdtlayer*>(dst_cell->securelayer(tdtlaynum));
                  if (check.box())  dwl->addbox(plist[0], plist[2], false);
                  else              dwl->addpoly(plist,false);
               }
               break;
            case gds_ENDEL://end of element, exit point
               return;
            default: //parse error - not expected record type
               throw EXPTNreadGDS("GDS box - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);

}

void GDSin::GdsStructure::importPoly(GdsInFile* cf, laydata::tdtcell* dst_cell, const LayerMapGds& theLayMap)
{
   int2b       layer;
   int2b       singleType;
   word        tdtlaynum;
   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_ELFLAGS: //readElflags(cr);// seems that it's not used
               break;
            case gds_PLEX:   //readPlex(cr);// seems that it's not used
               break;
            case gds_LAYER: cr->retData(&layer);
               break;
            case gds_DATATYPE: cr->retData(&singleType);
               break;
            case gds_PROPATTR: tell_log(console::MT_WARNING,"GDS boundary - PROPATTR record ignored");
               cf->incGdsiiWarnings(); break;
            case gds_PROPVALUE: tell_log(console::MT_WARNING,"GDS boundary - PROPVALUE record ignored");
               cf->incGdsiiWarnings(); break;
            case gds_XY:
               if ( theLayMap.getTdtLay(tdtlaynum, layer, singleType) )
               {
                  //one point less because fist and last point coincide
                  word numpoints = (cr->recLen())/8 - 1;
                  pointlist plist;
                  plist.reserve(numpoints);
                  for(word i = 0; i < numpoints; i++)
                     plist.push_back(GDSin::get_TP(cr, i));

                  laydata::valid_poly check(plist);

                  if (!check.valid())
                  {
                     std::ostringstream ost;
                     ost << "Polygon check fails - {" << check.failtype()
                         << " Layer: " << layer
                         << " Data type: " << singleType
                         << " }";
                     tell_log(console::MT_ERROR, ost.str());
                  }
                  laydata::tdtlayer* dwl = static_cast<laydata::tdtlayer*>(dst_cell->securelayer(tdtlaynum));
                  if (check.box())  dwl->addbox(plist[0], plist[2], false);
                  else              dwl->addpoly(plist,false);
               }
               break;
            case gds_ENDEL://end of element, exit point
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS boundary - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::importPath(GdsInFile* cf, laydata::tdtcell* dst_cell, const LayerMapGds& theLayMap)
{
   int2b layer;
   int2b singleType;
   int2b pathtype  = 0;
   int4b width     = 0;
   int4b bgnextn   = 0;
   int4b endextn   = 0;
   word  tdtlaynum;
   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_ELFLAGS: //readElflags(cr);// seems that's not used
               break;
            case gds_PLEX:   //readPlex(cr);// seems that's not used
               break;
            case gds_LAYER: cr->retData(&layer);
               break;
            case gds_DATATYPE: cr->retData(&singleType);
               break;
            case gds_PATHTYPE: cr->retData(&pathtype);
               break;
            case gds_WIDTH: cr->retData(&width);
               break;
            case gds_BGNEXTN:   cr->retData(&bgnextn);
               break;
            case gds_ENDEXTN:   cr->retData(&endextn);
               break;
            case gds_PROPATTR: tell_log(console::MT_WARNING,"GDS path - PROPATTR record ignored");
               cf->incGdsiiWarnings(); break;
            case gds_PROPVALUE: tell_log(console::MT_WARNING,"GDS path - PROPVALUE record ignored");
               cf->incGdsiiWarnings(); break;
            case gds_XY:
               if ( theLayMap.getTdtLay(tdtlaynum, layer, singleType) )
               {
                  word numpoints = (cr->recLen())/8;
                  pointlist plist;
                  bool pathConvertResult = true;
                  plist.reserve(numpoints);
                  for(word i = 0; i < numpoints; i++)
                     plist.push_back(GDSin::get_TP(cr, i));

                  if (2 == pathtype)
                     pathConvertResult = pathConvert(plist, numpoints, width/2, width/2);
                  else if (4 == pathtype)
                     pathConvertResult = pathConvert(plist, numpoints, bgnextn, endextn);

                  if (pathConvertResult)
                  {
                     laydata::valid_wire check(plist, width);

                     if (!check.valid())
                     {
                        std::ostringstream ost;
                        ost << "Wire check fails - {" << check.failtype()
                              << " Layer: " << layer
                              << " Data type: " << singleType
                              << " }";
                        tell_log(console::MT_ERROR, ost.str());
                     }
                     laydata::tdtlayer* dwl = static_cast<laydata::tdtlayer*>(dst_cell->securelayer(tdtlaynum));
                     dwl->addwire(plist, width,false);
                  }
                  else
                  {
                     std::ostringstream ost;
                     ost << "Invalid single point path - { Layer: " << layer
                           << " Data type: " << singleType
                           << " }";
                     tell_log(console::MT_ERROR, ost.str());
                  }
               }
               break;
            case gds_ENDEL://end of element, exit point
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS path - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (cr->recType() != gds_ENDEL);
}

void GDSin::GdsStructure::importText(GdsInFile* cf, laydata::tdtcell* dst_cell, real dbuu, const LayerMapGds& theLayMap)
{
   int2b       layer;
   int2b       singleType;
   word        ba;
   // initializing
   word        font           = 0;
   word        vertJust       = 0;
   word        horiJust       = 0;
   word        reflection     = 0;
   word        absMagn        = 0;
   word        absAngl        = 0;
   int2b       pathType       = 0;
   int4b       width          = 0;
   double      magnification  = 1.0;
   double      angle          = 0.0;
   TP          magnPoint;
   std::string tString;
   word        tdtlaynum;

   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_ELFLAGS: //readElflags(cr);// seems that it's not used
               break;
            case gds_PLEX:   //readPlex(cr);// seems that it's not used
               break;
            case gds_LAYER: cr->retData(&layer);
               break;
            case gds_TEXTTYPE: cr->retData(&singleType);
               break;
            case gds_PATHTYPE: cr->retData(&pathType);// ??? for test ???
               break;
            case gds_WIDTH: cr->retData(&width);// seems not to be used
               break;
            case gds_PROPATTR: tell_log(console::MT_WARNING,"GDS text - PROPATTR record ignored");
               cf->incGdsiiWarnings(); break;
            case gds_PROPVALUE: tell_log(console::MT_WARNING,"GDS text - PROPVALUE record ignored");
               cf->incGdsiiWarnings(); break;
            case gds_PRESENTATION:
               cr->retData(&ba,0,16);
               font = ba & 0x0030; font >>= 4;
               vertJust = ba & 0x000C; vertJust >>= 2;
               horiJust = ba & 0x0003;
               break;
            case gds_STRANS:
               cr->retData(&ba,0,16);
               reflection = ba & 0x8000; reflection >>= 15;//bit 0
               absMagn    = ba & 0x0004; absMagn    >>= 2; //bit 13
               absAngl    = ba & 0x0002; absAngl    >>= 1; //bit 14
               break;
            case gds_MAG: cr->retData(&magnification);
               break;
            case gds_ANGLE: cr->retData(&angle);
               break;
            case gds_XY: magnPoint = GDSin::get_TP(cr);
               break;
            case gds_STRING: cr->retData(&tString);
               break;
            case gds_ENDEL://end of element, exit point
               if ( theLayMap.getTdtLay(tdtlaynum, layer, singleType) )
               {
                  laydata::tdtlayer* dwl = static_cast<laydata::tdtlayer*>(dst_cell->securelayer(tdtlaynum));
                  // @FIXME absolute magnification, absolute angle should be reflected somehow!!!
                  dwl->addtext(tString,
                              CTM( magnPoint,
                                    magnification / (dbuu *  OPENGL_FONT_UNIT),
                                    angle,
                                 (0 != reflection)
                                 )
                              );
               }
               return;
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS text - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::importSref(GdsInFile* cf, laydata::tdtcell* dst_cell, laydata::tdtlibdir* tdt_db, const LayerMapGds&)
{
   word           reflection     = 0;
   word           absMagn        = 0;
   word           absAngl        = 0;
   double         magnification  = 1.0;
   double         angle          = 0.0;
   TP             magnPoint;
   std::string    strctName;
   word           ba;
   int            tmp; //Dummy variable. Use for gds_PROPATTR
   char           tmp2[128]; //Dummy variable. Use for gds_PROPVALUE
   std::ostringstream ost;
   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_ELFLAGS: //readElflags(cr);// seems that it's not used
               break;
            case gds_PLEX:   //readPlex(cr); // seems that it's not used
               break;
            case gds_SNAME:
               cr->retData(&strctName);
               break;
            case gds_STRANS:
               cr->retData(&ba,0,16);
               reflection  = ba & 0x8000;//mask all bits except 0
               absMagn     = ba & 0x0004;//mask all bits except 13
               absAngl     = ba & 0x0002;//mask all bits except 14
               break;
            case gds_MAG: cr->retData(&magnification);
               break;
            case gds_ANGLE: cr->retData(&angle);
               break;
            case gds_XY: magnPoint = GDSin::get_TP(cr);
               break;
            case gds_PROPATTR:
               cr->retData(&tmp);
               break;
            case gds_PROPVALUE:
               cr->retData(&tmp2);
               ost << "Property attribute  " << tmp << " with value \"" << tmp2 << "\" ignored" ; break;
               tell_log(console::MT_WARNING, ost.str());
               cf->incGdsiiWarnings();
               break;
            case gds_ENDEL:{//end of element, exit point
               // @FIXME absolute magnification, absolute angle should be reflected somehow!!!
               laydata::CellDefin strdefn = tdt_db->linkcellref(strctName, TARGETDB_LIB);
               dst_cell->registerCellRef( strdefn,
                                          CTM(magnPoint,
                                              magnification,
                                              angle,
                                              (0 != reflection)
                                             )
                                        );
               return;
            }
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS sref - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

void GDSin::GdsStructure::importAref(GdsInFile* cf, laydata::tdtcell* dst_cell, laydata::tdtlibdir* tdt_db, const LayerMapGds&)
{
   word           reflection     = 0;
   word           absMagn        = 0;
   word           absAngl        = 0;
   double         magnification  = 1.0;
   double         angle          = 0.0;
   int2b          columns        = 0;
   int2b          rows           = 0;
   TP             magnPoint;
   TP             xStep;
   TP             yStep;
   std::string    strctName;
   word ba;
   int tmp; //Dummy variable. Use for gds_PROPATTR
   char tmp2[128]; //Dummy variable. Use for gds_PROPVALUE
   std::ostringstream ost;
   //initializing
   const GdsRecord* cr = cf->cRecord();
   do
   {//start reading
      if (cf->getNextRecord())
      {
         switch (cr->recType())
         {
            case gds_ELFLAGS://readElflags(cr);// seems that it's not used
               break;
            case gds_PLEX://readPlex(cr);// seems that it's not used
               break;
            case gds_SNAME:
               cr->retData(&strctName);
               break;
            case gds_STRANS:
               cr->retData(&ba,0,16);
               reflection  = ba & 0x8000;//mask all bits except 0
               absMagn     = ba & 0x0004;//mask all bita except 13
               absAngl     = ba & 0x0002;//mask all bita except 14
               break;
            case gds_MAG: cr->retData(&magnification);
               break;
            case gds_ANGLE: cr->retData(&angle);
               break;
            case gds_XY:
               magnPoint = GDSin::get_TP(cr,0);
               xStep     = GDSin::get_TP(cr,1);
               yStep     = GDSin::get_TP(cr,2);
               break;
            case gds_COLROW://return number of columns & rows in the array
               cr->retData(&columns  );
               cr->retData(&rows   ,2);
               break;
            case gds_PROPATTR:
               cr->retData(&tmp);
               break;
            case gds_PROPVALUE:
               cr->retData(&tmp2);
               ost << "Property attribute  " << tmp << " with value \"" << tmp2 << "\" ignored" ; break;
               tell_log(console::MT_WARNING, ost.str());
               cf->incGdsiiWarnings();
               break;
            case gds_ENDEL:{//end of element, exit point
               // Absolute magnification, absolute angle should be reflected somehow!!!
               laydata::CellDefin strdefn = tdt_db->linkcellref(strctName, TARGETDB_LIB);

               laydata::ArrayProperties arrprops(arrGetStep(xStep, magnPoint, columns),
                                                 arrGetStep(yStep, magnPoint, rows   ),
                                                 static_cast<word>(columns),
                                                 static_cast<word>(rows)
                                                );
               dst_cell->registerCellARef( strdefn,
                                           CTM( magnPoint,
                                                magnification,
                                                angle,
                                                (0 != reflection)
                                              ),
                                           arrprops
                                         );
               return;
            }
            default://parse error - not expected record type
               throw EXPTNreadGDS("GDS aref - wrong record type in the current context");
         }
      }
      else
         throw EXPTNreadGDS("Unexpected end of file");
   }
   while (true);
}

bool GDSin::GdsStructure::pathConvert(pointlist& plist, word numpoints, int4b begext, int4b endext )
{
   TP P1 = plist[0];
   // find the first neighbouring point which is not equivaqlent to P1
   int fnbr = 1;
   while ((P1 == plist[fnbr]) && (fnbr < numpoints))
      fnbr++;
   // get out with error, because the wire has effectively a single point and there is
   // no way on earth to find out in which direction it should be expanded
   if (fnbr == numpoints) return false;
   TP P2 = plist[fnbr];

   double sdX = P2.x() - P1.x();
   double sdY = P2.y() - P1.y();
   // The sign - a bit funny way - described in layout canvas
   int sign = ((sdX * sdY) >= 0) ? 1 : -1;
   double length = sqrt(sdY*sdY + sdX*sdX);
   assert(length);
   int4b y0 = (int4b) rint(P1.y() - sign*((begext*sdY)/length));
   int4b x0 = (int4b) rint(P1.x() - sign*((begext*sdX)/length));
//
   P2 = plist[numpoints-1];
   // find the first neighbouring point which is not equivaqlent to P1
   fnbr = numpoints - 2;
   while ((P2 == plist[fnbr]) && (fnbr > 0))
      fnbr--;
   // assert, because if it was found above, it should exists!
   assert(fnbr >= 0);
   P1 = plist[fnbr];

   P1 = plist[numpoints-2];
   sdX = P2.x() - P1.x();
   sdY = P2.y() - P1.y();
   sign = ((sdX * sdY) >= 0) ? 1 : -1;
   length = sqrt(sdY*sdY + sdX*sdX);
   int4b yn = (int4b) rint(P2.y() + sign*((endext*sdY)/length));
   int4b xn = (int4b) rint(P2.x() + sign*((endext*sdX)/length));

   plist[0].setX(x0);
   plist[0].setY(y0);
   plist[numpoints-1].setX(xn);
   plist[numpoints-1].setY(yn);

   return true;
}

int GDSin::GdsStructure::arrGetStep(TP& Step, TP& magnPoint, int2b colrows)
{
   return (int) sqrt(pow(float((Step.x() - magnPoint.x())),2) +
                     pow(float((Step.y() - magnPoint.y())),2)   ) / colrows;
}

void GDSin::GdsStructure::split(GdsInFile* src_file, GdsOutFile* dst_file)
{
   const GdsRecord* cr = src_file->cRecord();
   src_file->setPosition(_filePos - _beginRecLength);
   wxFileOffset endPosition = dst_file->filePos() + _strSize + _beginRecLength;
   do
   { //start reading
      src_file->getNextRecord();
      dst_file->putRecord(cr);
   } while (dst_file->filePos() < endPosition);
}

GDSin::GdsStructure::~GdsStructure()
{

}

//-----------------------------------------------------------------------------
// class Gds2Ted
//-----------------------------------------------------------------------------
GDSin::Gds2Ted::Gds2Ted(GDSin::GdsInFile* src_lib, laydata::tdtlibdir* tdt_db, const LayerMapGds& theLayMap) :
      _src_lib(src_lib), _tdt_db(tdt_db), _theLayMap(theLayMap),
               _coeff((*_tdt_db)()->UU() / src_lib->libUnits()), _conversionLength(0)
{}

void GDSin::Gds2Ted::run(const nameList& top_str_names, bool recursive, bool overwrite)
{
   assert(_src_lib->hierTree());

   for (nameList::const_iterator CN = top_str_names.begin(); CN != top_str_names.end(); CN++)
   {
      GDSin::GdsStructure *src_structure = _src_lib->getStructure(*CN);
      if (NULL != src_structure)
      {
         GDSin::GDSHierTree* root = _src_lib->hierTree()->GetMember(src_structure);
         if (recursive) preTraverseChildren(root);
         if (!src_structure->traversed())
         {
            _convertList.push_back(src_structure);
            src_structure->set_traversed(true);
            _conversionLength += src_structure->strSize();
         }
      }
      else
      {
         std::ostringstream ost; ost << "GDS import: ";
         ost << "Structure \""<< *CN << "\" not found in the GDS DB.";
         tell_log(console::MT_WARNING,ost.str());
      }
   }
   if (_src_lib->reopenFile())
   {
      TpdPost::toped_status(console::TSTS_PRGRSBARON, _conversionLength);
      try
      {
         for (GDSStructureList::iterator CS = _convertList.begin(); CS != _convertList.end(); CS++)
         {
            convert(*CS, overwrite);
            (*CS)->set_traversed(false); // restore the state for eventual second conversion
         }
         tell_log(console::MT_INFO, "Done");
      }
      catch (EXPTNreadGDS) {tell_log(console::MT_INFO, "Conversion aborted with errors");}
      TpdPost::toped_status(console::TSTS_PRGRSBAROFF);
      _src_lib->closeFile();
      (*_tdt_db)()->recreate_hierarchy(_tdt_db);
   }
}

void GDSin::Gds2Ted::preTraverseChildren(const GDSin::GDSHierTree* root)
{
   const GDSin::GDSHierTree* Child = root->GetChild(TARGETDB_LIB);
   while (Child)
   {
      if ( !Child->GetItem()->traversed() )
      {
         // traverse children first
         preTraverseChildren(Child);
         GDSin::GdsStructure* sstr = const_cast<GDSin::GdsStructure*>(Child->GetItem());
         if (!sstr->traversed())
         {
            _convertList.push_back(sstr);
            sstr->set_traversed(true);
            _conversionLength += sstr->strSize();
         }
      }
      Child = Child->GetBrother(TARGETDB_LIB);
   }
}

void GDSin::Gds2Ted::convert(GDSin::GdsStructure* src_structure, bool overwrite)
{
   std::string gname = src_structure->strctName();
   // check that destination structure with this name exists
   laydata::tdtcell* dst_structure = static_cast<laydata::tdtcell*>((*_tdt_db)()->checkcell(gname));
   std::ostringstream ost; ost << "GDS import: ";
   if (NULL != dst_structure)
   {
      if (overwrite)
      {
         /*@TODO Erase the existing structure and convert*/
         ost << "Structure "<< gname << " should be overwritten, but cell erase is not implemened yet ...";
         tell_log(console::MT_WARNING,ost.str());
      }
      else
      {
         ost << "Structure "<< gname << " already exists. Skipped";
         tell_log(console::MT_INFO,ost.str());
      }
   }
   else
   {
      ost << "Structure " << gname << "...";
      tell_log(console::MT_INFO,ost.str());
      // first create a new cell
      dst_structure = DEBUG_NEW laydata::tdtcell(gname);
      // call the cell converter
      src_structure->import(_src_lib, dst_structure, _tdt_db, _theLayMap);
      // and finally - register the cell
      (*_tdt_db)()->registercellread(gname, dst_structure);
   }
}

//-----------------------------------------------------------------------------
// class GdsOutFile
//-----------------------------------------------------------------------------

GDSin::GdsOutFile::GdsOutFile(std::string fileName)
{
   _filePos = 0;
   _streamVersion = 3;
   wxString wxfname(fileName.c_str(), wxConvUTF8 );
   _gdsFh.Open(wxfname.c_str(),wxT("wb"));
   if (!(_gdsFh.IsOpened()))
   {// open the output file
      std::ostringstream info;
      info << "File "<< fileName <<" can NOT be opened";
      tell_log(console::MT_ERROR,info.str());
      //@TODO: Exception Here
      return;
   }//
   // start writing
   GdsRecord* wr = NULL;
   // ... GDS header
   wr = setNextRecord(gds_HEADER); wr->add_int2b(_streamVersion);
   flush(wr);
}

GDSin::GdsOutFile::~GdsOutFile()
{
   if (_gdsFh.IsOpened())
      _gdsFh.Close();
}

GDSin::GdsRecord* GDSin::GdsOutFile::setNextRecord(byte rectype, word reclen)
{
   byte datatype;
   switch (rectype)
   {
      case gds_HEADER         :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT2B   , 2         );
      case gds_BGNLIB         :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT2B   , 24        );
      case gds_ENDLIB         :return DEBUG_NEW GdsRecord(rectype, gdsDT_NODATA  , 0         );
      case gds_LIBNAME        :return DEBUG_NEW GdsRecord(rectype, gdsDT_ASCII   , reclen    );
      case gds_UNITS          :return DEBUG_NEW GdsRecord(rectype, gdsDT_REAL8B  , 16        );
      case gds_BGNSTR         :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT2B   , 24        );
      case gds_STRNAME        :return DEBUG_NEW GdsRecord(rectype, gdsDT_ASCII   , reclen    );
      case gds_ENDSTR         :return DEBUG_NEW GdsRecord(rectype, gdsDT_NODATA  , 0         );
      case gds_BOUNDARY       :return DEBUG_NEW GdsRecord(rectype, gdsDT_NODATA  , 0         );
      case gds_PATH           :return DEBUG_NEW GdsRecord(rectype, gdsDT_NODATA  , 0         );
      case gds_SREF           :return DEBUG_NEW GdsRecord(rectype, gdsDT_NODATA  , 0         );
      case gds_AREF           :return DEBUG_NEW GdsRecord(rectype, gdsDT_NODATA  , 0         );
      case gds_TEXT           :return DEBUG_NEW GdsRecord(rectype, gdsDT_NODATA  , 0         );
      case gds_LAYER          :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT2B   , 2         );
      case gds_DATATYPE       :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT2B   , 2         );
      case gds_XY             :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT4B   , 8*reclen  );
      case gds_WIDTH          :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT4B   , 4         );
      case gds_ENDEL          :return DEBUG_NEW GdsRecord(rectype, gdsDT_NODATA  , 0         );
      case gds_SNAME          :return DEBUG_NEW GdsRecord(rectype, gdsDT_ASCII   , reclen    );
      case gds_COLROW         :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT2B   , 4         );
      case gds_TEXTTYPE       :return DEBUG_NEW GdsRecord(rectype, gdsDT_INT2B   , 2         );
      case gds_STRING         :return DEBUG_NEW GdsRecord(rectype, gdsDT_ASCII   , reclen    );
      case gds_STRANS         :return DEBUG_NEW GdsRecord(rectype, gdsDT_BIT     , 2         );
      case gds_MAG            :return DEBUG_NEW GdsRecord(rectype, gdsDT_REAL8B  , 8         );
      case gds_ANGLE          :return DEBUG_NEW GdsRecord(rectype, gdsDT_REAL8B  , 8         );
      case gds_PROPATTR       :datatype = gdsDT_INT2B;break;
      case gds_PROPVALUE      :datatype = gdsDT_ASCII;break;
                       default: assert(false); //the rest should not be used
//----------------------------------------------------------------------------------
// The record types below are not used currently in GDS export
//       case gds_NODE           :datatype = gdsDT_NODATA;break;
//       case gds_PRESENTATION   :datatype = gdsDT_BIT;break;
//       case gds_REFLIBS        :datatype = gdsDT_ASCII;break;
//       case gds_FONTS          :datatype = gdsDT_ASCII;break;
//       case gds_PATHTYPE       :datatype = gdsDT_INT2B;break;
//       case gds_GENERATION     :datatype = gdsDT_INT2B;break;
//       case gds_ATTRTABLE      :datatype = gdsDT_ASCII;break;
//       case gds_ELFLAGS        :datatype = gdsDT_BIT;break;
//       case gds_NODETYPE       :datatype = gdsDT_INT2B;break;
//       case gds_PROPATTR       :datatype = gdsDT_INT2B;break;//
//       case gds_PROPVALUE      :datatype = gdsDT_ASCII;break;//
//       case gds_FORMAT         :datatype = gdsDT_INT2B;break;
//       case gds_BORDER         :datatype = gdsDT_NODATA;break;
//       case gds_SOFTFENCE      :datatype = gdsDT_NODATA;break;
//       case gds_HARDFENCE      :datatype = gdsDT_NODATA;break;
//       case gds_SOFTWIRE       :datatype = gdsDT_NODATA;break;
//       case gds_HARDWIRE       :datatype = gdsDT_NODATA;break;
//       case gds_PATHPORT       :datatype = gdsDT_NODATA;break;
//       case gds_NODEPORT       :datatype = gdsDT_NODATA;break;
//       case gds_USERCONSTRAINT :datatype = gdsDT_NODATA;break;
//       case gds_SPACER_ERROR   :datatype = gdsDT_NODATA;break;
//       case gds_CONTACT        :datatype = gdsDT_NODATA;break;


//      deprecated, should not be used for export
//      case gds_BOX            :datatype = gdsDT_NODATA;break;
//      case gds_BOXTYPE        :datatype = gdsDT_INT2B;break;
//      case gds_PLEX           :datatype = gdsDT_INT4B;break;
//      case gds_BGNEXTN        :datatype = gdsDT_INT4B;break;
//      case gds_ENDEXTN        :datatype = gdsDT_INT4B;break;
//      case gds_MASK           :datatype = gdsDT_ASCII;break;
//      case gds_ENDMASKS       :datatype = gdsDT_NODATA;break;
//      case gds_LIBSECUR       :datatype = gdsDT_INT2B;break;
//      case gds_SRFNAME        :datatype = gdsDT_ASCII;break;
//      case gds_LIBDIRSIZE     :datatype = gdsDT_INT2B;break;
//      case gds_TEXTNODE       :datatype = gdsDT_NODATA;break;
//      case gds_STYPTABLE      :datatype = gdsDT_ASCII;break;
//      case gds_STRTYPE        :datatype = gdsDT_INT2B;break;
//      case gds_ELKEY          :datatype = gdsDT_INT4B;break;
//      case gds_STRCLASS       :datatype = gdsDT_BIT;break;
//      case gds_RESERVED       :datatype = gdsDT_INT4B;break;
//      case gds_TAPENUM        :datatype = gdsDT_INT2B;break;
//      case gds_TAPECODE       :datatype = gdsDT_INT2B;break;
//      case gds_SPACING:
//      case gds_UINTEGER:
//      case gds_USTRING:
//      case gds_LINKTYPE:
//      case gds_LINKKEYS:

   }
   return DEBUG_NEW GdsRecord(rectype, datatype,0);
}

void GDSin::GdsOutFile::flush(GdsRecord* wr)
{
   wr->flush(_gdsFh); delete wr;
}

void GDSin::GdsOutFile::setTimes(GdsRecord* wr)
{
   wr->add_int2b(_tModif.Year);
   wr->add_int2b(_tModif.Month);
   wr->add_int2b(_tModif.Day);
   wr->add_int2b(_tModif.Hour);
   wr->add_int2b(_tModif.Min);
   wr->add_int2b(_tModif.Sec);
   wr->add_int2b(_tAccess.Year);
   wr->add_int2b(_tAccess.Month);
   wr->add_int2b(_tAccess.Day);
   wr->add_int2b(_tAccess.Hour);
   wr->add_int2b(_tAccess.Min);
   wr->add_int2b(_tAccess.Sec);
}

void GDSin::GdsOutFile::timeSetup(const TpdTime& libtime)
{
   time_t acctim_N = libtime.stdCTime();
   tm* broken_time = localtime(&acctim_N);
   if (broken_time == NULL)
   {
      std::ostringstream info;
      info << "Error during defining time";
      tell_log(console::MT_ERROR,info.str());
   }
   _tAccess.Year  = broken_time->tm_year + 1900;
   _tAccess.Month = broken_time->tm_mon+1;
   _tAccess.Day   = broken_time->tm_mday;
   _tAccess.Hour  = broken_time->tm_hour;
   _tAccess.Min   = broken_time->tm_min;
   _tAccess.Sec   = broken_time->tm_sec;
   time_t cur_time = time(NULL);
   broken_time   = localtime(&cur_time);
   _tModif.Year   = broken_time->tm_year + 1900;
   _tModif.Month  = broken_time->tm_mon+1;
   _tModif.Day    = broken_time->tm_mday;
   _tModif.Hour   = broken_time->tm_hour;
   _tModif.Min    = broken_time->tm_min;
   _tModif.Sec    = broken_time->tm_sec;
}

void GDSin::GdsOutFile::putRecord(const GdsRecord* wr)
{
   word length = wr->recLen() + 4;
   byte  record[4];
   record[0] = ((byte*)&length)[1];
   record[1] = ((byte*)&length)[0];
   record[2] = wr->recType();
   record[3] = wr->dataType();

   size_t bytes_written = _gdsFh.Write(record, 4);
   _filePos += bytes_written;
   if (wr->recLen() > 0)
   {
      bytes_written = _gdsFh.Write(wr->record(), wr->recLen());
      _filePos += bytes_written;
   }
   delete wr;
}

void GDSin::GdsOutFile::updateLastRecord()
{
   word num_zeroes = 2048 - (_filePos % 2048);
   byte record = 0x00;
   size_t bytes_written = _gdsFh.Write(&record, num_zeroes);
   assert(bytes_written == num_zeroes);
   _filePos += bytes_written;
}

//-----------------------------------------------------------------------------
// class GdsExportFile
//-----------------------------------------------------------------------------
GDSin::GdsExportFile::GdsExportFile(std::string fn, laydata::tdtcell* tcell,
   const LayerMapGds& lmap, bool recur) : DbExportFile(fn, tcell, recur), GdsOutFile(fn), _laymap(lmap)
{
}

void GDSin::GdsExportFile::libraryStart(std::string libname, TpdTime& libtime, real DBU, real UU)
{
   timeSetup(libtime);
   //write BGNLIB record
   GdsRecord* wr = setNextRecord(gds_BGNLIB);
   setTimes(wr); flush(wr);

   wr = setNextRecord(gds_LIBNAME, libname.size());
   wr->add_ascii(libname.c_str()); flush(wr);
   wr = setNextRecord(gds_UNITS);
   wr->add_real8b(UU); wr->add_real8b(DBU);
   flush(wr);
}

void GDSin::GdsExportFile::libraryFinish()
{
   GdsRecord* wr = setNextRecord(gds_ENDLIB);
   flush(wr);
}

void GDSin::GdsExportFile::definitionStart(std::string cname)
{
   _ccname = cname;
   std::string message = "...converting " + _ccname;
   tell_log(console::MT_INFO, message);

   GDSin::GdsRecord* wr = setNextRecord(gds_BGNSTR);
   setTimes(wr);flush(wr);
   wr = setNextRecord(gds_STRNAME, _ccname.size());
   wr->add_ascii(_ccname.c_str()); flush(wr);

}

void GDSin::GdsExportFile::definitionFinish()
{
   GdsRecord* wr = setNextRecord(gds_ENDSTR);flush(wr);
   registerCellWritten(_ccname);

}

bool GDSin::GdsExportFile::layerSpecification(unsigned layno)
{
   return (getMappedLayType(_cGdsLayer, _cGdsType, layno));
}

void GDSin::GdsExportFile::box(const int4b* const pdata)
{
   GDSin::GdsRecord* wr = setNextRecord(gds_BOUNDARY);
   flush(wr);
   wr = setNextRecord(gds_LAYER);
   wr->add_int2b(_cGdsLayer);flush(wr);
   wr = setNextRecord(gds_DATATYPE);
   wr->add_int2b(_cGdsType);flush(wr);
   wr = setNextRecord(gds_XY,5);
   wr->add_int4b(pdata[0]);wr->add_int4b(pdata[1]);
   wr->add_int4b(pdata[2]);wr->add_int4b(pdata[3]);
   wr->add_int4b(pdata[4]);wr->add_int4b(pdata[5]);
   wr->add_int4b(pdata[6]);wr->add_int4b(pdata[7]);
   wr->add_int4b(pdata[0]);wr->add_int4b(pdata[1]);
   flush(wr);
   wr = setNextRecord(gds_ENDEL);
   flush(wr);
}

void GDSin::GdsExportFile::polygon(const int4b* const pdata, unsigned psize)
{
   GDSin::GdsRecord* wr = setNextRecord(gds_BOUNDARY);
   flush(wr);
   wr = setNextRecord(gds_LAYER);
   wr->add_int2b(_cGdsLayer);flush(wr);
   wr = setNextRecord(gds_DATATYPE);
   wr->add_int2b(_cGdsType);flush(wr);
   wr = setNextRecord(gds_XY,psize+1);
   for (word i = 0; i < psize; i++)
   {
      wr->add_int4b(pdata[2*i]);wr->add_int4b(pdata[2*i+1]);
   }
   wr->add_int4b(pdata[0]);wr->add_int4b(pdata[1]);
   flush(wr);
   wr = setNextRecord(gds_ENDEL);
   flush(wr);
}

void GDSin::GdsExportFile::wire(const int4b* const pdata, unsigned psize, unsigned width)
{
   GDSin::GdsRecord* wr = setNextRecord(gds_PATH);
   flush(wr);
   wr = setNextRecord(gds_LAYER);
   wr->add_int2b(_cGdsLayer);flush(wr);
   wr = setNextRecord(gds_DATATYPE);
   wr->add_int2b(_cGdsType);flush(wr);
   wr = setNextRecord(gds_WIDTH);
   wr->add_int4b(width);flush(wr);
   wr = setNextRecord(gds_XY,psize);
   for (word i = 0; i < psize; i++)
   {
      wr->add_int4b(pdata[2*i]);wr->add_int4b(pdata[2*i+1]);
   }
   flush(wr);
   wr = setNextRecord(gds_ENDEL);
   flush(wr);
}

void GDSin::GdsExportFile::text(const std::string& text, const CTM& trans)
{
   GDSin::GdsRecord* wr = setNextRecord(gds_TEXT);
   flush(wr);
   wr = setNextRecord(gds_LAYER);
   wr->add_int2b(_cGdsLayer);flush(wr);
   wr = setNextRecord(gds_TEXTTYPE);
   wr->add_int2b(_cGdsType);flush(wr);
   TP bind;
   real rotation, scale;
   bool flipX;
   trans.Decompose(bind,rotation,scale,flipX);
   wr = setNextRecord(gds_STRANS);
   if (flipX) wr->add_int2b(0x8000);
   else       wr->add_int2b(0x0000);
   flush(wr);
   wr = setNextRecord(gds_MAG);
   wr->add_real8b(scale * OPENGL_FONT_UNIT * UU());flush(wr);
   wr = setNextRecord(gds_ANGLE);
   wr->add_real8b(rotation);flush(wr);
   wr = setNextRecord(gds_XY,1);
   wr->add_int4b(bind.x());wr->add_int4b(bind.y());
   flush(wr);
   wr = setNextRecord(gds_STRING, text.size());
   wr->add_ascii(text.c_str());flush(wr);
   wr = setNextRecord(gds_ENDEL);
   flush(wr);
}

void GDSin::GdsExportFile::ref(const std::string& name, const CTM& translation)
{
   GDSin::GdsRecord* wr = setNextRecord(gds_SREF);
   flush(wr);
   wr = setNextRecord(gds_SNAME, name.size());
   wr->add_ascii(name.c_str());flush(wr);
   TP trans;
   real rotation, scale;
   bool flipX;
   translation.Decompose(trans,rotation,scale,flipX);
   wr = setNextRecord(gds_STRANS);
   if (flipX) wr->add_int2b(0x8000);
   else       wr->add_int2b(0x0000);
   flush(wr);
   wr = setNextRecord(gds_MAG);
   wr->add_real8b(scale);flush(wr);
   wr = setNextRecord(gds_ANGLE);
   wr->add_real8b(rotation);flush(wr);
   wr = setNextRecord(gds_XY,1);
   wr->add_int4b(trans.x());wr->add_int4b(trans.y());
   flush(wr);
   wr = setNextRecord(gds_ENDEL);
   flush(wr);
}

void GDSin::GdsExportFile::aref(const std::string& name, const CTM& translation,
                                const laydata::ArrayProperties& arrprops)
{
   GDSin::GdsRecord* wr = setNextRecord(gds_AREF);
   flush(wr);
   wr = setNextRecord(gds_SNAME, name.size());
   wr->add_ascii(name.c_str());flush(wr);
   TP trans;
   real rotation, scale;
   bool flipX;
   translation.Decompose(trans,rotation,scale,flipX);
   wr = setNextRecord(gds_STRANS);
   if (flipX) wr->add_int2b(0x8000);
   else       wr->add_int2b(0x0000);
   flush(wr);
   wr = setNextRecord(gds_MAG);
   wr->add_real8b(scale);flush(wr);
   wr = setNextRecord(gds_ANGLE);
   wr->add_real8b(rotation);flush(wr);
   wr = setNextRecord(gds_COLROW);
   wr->add_int2b(arrprops.cols());wr->add_int2b(arrprops.rows());
   flush(wr);
   wr = setNextRecord(gds_XY,3);
   wr->add_int4b(trans.x());wr->add_int4b(trans.y());
   wr->add_int4b(trans.x() + arrprops.cols() * arrprops.stepX());wr->add_int4b(trans.y());
   wr->add_int4b(trans.x());wr->add_int4b(trans.y() + arrprops.rows() * arrprops.stepY());
   flush(wr);
   wr = setNextRecord(gds_ENDEL);
   flush(wr);
}

bool GDSin::GdsExportFile::checkCellWritten(std::string cellname) const
{
   for (nameList::const_iterator i = _childnames.begin();
                                 i != _childnames.end(); i++)
      if (cellname == *i) return true;
   return false;
}

void GDSin::GdsExportFile::registerCellWritten(std::string cellname)
{
   _childnames.push_back(cellname);
}

bool GDSin::GdsExportFile::getMappedLayType(word& gdslay, word& gdstype, word tdtlay)
{
   bool result = _laymap.getGdsLayType(gdslay, gdstype, tdtlay);
   return result;
   //It should not be a problem if the tdtlay is not listed in the map. Then
   // we take the default mapping which is gdslay = tdtlay; gdstype = 0
}

GDSin::GdsExportFile::~GdsExportFile()
{
}

//-----------------------------------------------------------------------------
GDSin::GdsSplit::GdsSplit(GDSin::GdsInFile* src_lib, std::string dst_file_name) : _src_lib(src_lib)
{
   _dst_lib = DEBUG_NEW GDSin::GdsOutFile(dst_file_name);
}

void GDSin::GdsSplit::run(GDSin::GdsStructure* src_structure, bool recursive)
{
   assert(_src_lib->hierTree());
   assert(src_structure);

   GDSin::GDSHierTree* root = _src_lib->hierTree()->GetMember(src_structure);
   if (recursive) preTraverseChildren(root);
   if (!src_structure->traversed())
   {
      _convertList.push_back(src_structure);
      src_structure->set_traversed(true);
   }

   if (_src_lib->reopenFile())
   {
      GDSin::GdsRecord* wr = _dst_lib->setNextRecord(gds_LIBNAME, src_structure->strctName().size());
      wr->add_ascii(src_structure->strctName().c_str()); _dst_lib->flush(wr);

      wr = _dst_lib->setNextRecord(gds_UNITS);
      wr->add_real8b(_src_lib->library()->uu()); wr->add_real8b(_src_lib->library()->dbu());
      _dst_lib->flush(wr);

      for (GDSStructureList::iterator CS = _convertList.begin(); CS != _convertList.end(); CS++)
      {
         split(*CS);
         (*CS)->set_traversed(false); // restore the state for eventual second conversion
      }

      wr = _dst_lib->setNextRecord(gds_ENDLIB);_dst_lib->flush(wr);

      tell_log(console::MT_INFO, "Done");
      _src_lib->closeFile();
   }
}

void GDSin::GdsSplit::preTraverseChildren(const GDSin::GDSHierTree* root)
{
   const GDSin::GDSHierTree* Child = root->GetChild(TARGETDB_LIB);
   while (Child)
   {
      if ( !Child->GetItem()->traversed() )
      {
         // traverse children first
         preTraverseChildren(Child);
         GDSin::GdsStructure* sstr = const_cast<GDSin::GdsStructure*>(Child->GetItem());
         if (!sstr->traversed())
         {
            _convertList.push_back(sstr);
            sstr->set_traversed(true);
         }
      }
      Child = Child->GetBrother(TARGETDB_LIB);
   }
}

void GDSin::GdsSplit::split(GDSin::GdsStructure* src_structure)
{
   std::string gname = src_structure->strctName();
   std::ostringstream ost; ost << "GDS split: Writing structure " << gname << "...";
   tell_log(console::MT_INFO,ost.str());
   // finally call the split writer
   src_structure->split(_src_lib, _dst_lib);
}

GDSin::GdsSplit::~GdsSplit()
{
   delete _dst_lib;
}

//-----------------------------------------------------------------------------
TP GDSin::get_TP(const GDSin::GdsRecord *cr, word curnum, byte len)
{
   int4b GDS_X, GDS_Y;
   cr->retData(&GDS_X, curnum*len*2, len);
   cr->retData(&GDS_Y, curnum*len*2+len, len);
   return TP(GDS_X,GDS_Y);
}
