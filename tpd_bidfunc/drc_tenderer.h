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
//   This file is a part of Toped project (C) 2001-2012 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Mon Mar 02 2009
//     Originator: Sergey Gaitukevich - gaitukevich.s@toped.org.uk
//    Description: Interlayer between CalbrFile and Toped database
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
//      Comments :
//===========================================================================

#if !defined(DRC_TENDERER_H_INCLUDED)
#define DRC_TENDERER_H_INCLUDED

#include "calbr_reader.h"

//namespace Calbr
//{
//   class drcTenderer: public drcRenderer
//   {
//      public:
//                           drcTenderer(Calbr::DrcLibrary* library);
//      virtual             ~drcTenderer();
//      void                 setError(unsigned int numError);
//      void                 startWriting(const std::string &cell);
//      void                 addPoly(const CoordsVector   &coords);
//      void                 addLine(const edge &edge);
//      void                 showAll(void);
//      void                 hideAll(void);
//      bool                 showError(unsigned int numError);
//      void                 zoom(const edge &edge);
//      void                 endWriting();
//      bool                 checkCellName();
//   private:
//      Calbr::DrcLibrary*   _ATDB;
//      laydata::TdtCell*    _DRCCell;
//      std::string          _cell;
//      unsigned int         _numError;
//      TP                   _max;
//      TP                   _min;
//      bool                 _startDrawing; //use for initial setting of _minx, maxy etc
//   };
//
//}

#endif //DRC_TENDERER_H_INCLUDED
