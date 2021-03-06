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
//        Created: Sat Mar 26 13:48:39 GMT 2011
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Handling of objects failing Graphical Rules Checks
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef AUXDAT_H_
#define AUXDAT_H_

#include "qtree_tmpl.h"

namespace auxdata {

   //==============================================================================
   class AuxData  {
   public:
      //! The default constructor.
                           AuxData(SH_STATUS sel) : _status(sel) {};
      virtual             ~AuxData(){};
      //! Return the overlapping box of the object.
      virtual   DBbox      overlap()  const = 0;
      //! Draw request (rendering)
      virtual   void       drawRequest(trend::TrendBase&) const = 0;
      //! Draw request (rendering) - if the object is selected
      virtual   void       drawSRequest(trend::TrendBase&, const SGBitSet*) const = 0;
      //! Draw the objects in motion during copy/move and similar operations
      virtual   void       motionDraw(const layprop::DrawProperties&, CtmQueue&, SGBitSet*) const = 0;
      //! Print an object description on the toped console.
      virtual   void       info(std::ostringstream&, real) const = 0;
      //!
      virtual   bool       pointInside(const TP) const = 0;
      //!
      void                 setStatus(SH_STATUS s) {_status = s;}
      SH_STATUS            status() const {return _status;}
   protected:
      SH_STATUS            _status;
   };


   //==============================================================================
   class GrcData : public AuxData {
   public:
      //! The default constructor.
                           GrcData(SH_STATUS sel=sh_invalid) : AuxData(sel){};
      virtual             ~GrcData(){};
      //! Print an object description on the toped console.
      virtual   void       write(OutputTdtFile* const tedfile) const = 0;
      //! Export the TdtData object in external format.
      virtual   void       dbExport(DbExportFile&) const = 0;
      //!
      virtual  PointVector dumpPoints() const = 0;
      //!
      virtual  laydata::ShapeList* getRepaired() const = 0;
   };

   //==============================================================================
   class TdtGrcPoly : public GrcData   {
      public:
                           TdtGrcPoly(const PointVector&);
                           TdtGrcPoly(int4b*, unsigned);
                           TdtGrcPoly(InputTdtFile* const);
         virtual          ~TdtGrcPoly();
         virtual DBbox     overlap() const;

         virtual void      drawRequest(trend::TrendBase&) const;
         virtual void      drawSRequest(trend::TrendBase&, const SGBitSet*) const;
         virtual void      motionDraw(const layprop::DrawProperties&, CtmQueue&, SGBitSet*) const;

         virtual void      info(std::ostringstream&, real) const;
         virtual void      write(OutputTdtFile* const tedfile) const;
         virtual void      dbExport(DbExportFile&) const;
//         virtual word      numPoints() const {return _psize;}
         virtual bool      pointInside(const TP)const;
         virtual PointVector dumpPoints() const;
         virtual laydata::ShapeList* getRepaired() const;
      private:
         int4b*            _pdata;
         unsigned          _psize;
   };

   //==============================================================================
   class TdtGrcWire : public GrcData   {
      public:
                           TdtGrcWire(const PointVector&, WireWidth);
                           TdtGrcWire(int4b*, unsigned, WireWidth);
                           TdtGrcWire(InputTdtFile* const);
         virtual          ~TdtGrcWire();
         virtual DBbox     overlap() const;

         virtual void      drawRequest(trend::TrendBase&) const;
         virtual void      drawSRequest(trend::TrendBase&, const SGBitSet*) const;
         virtual void      motionDraw(const layprop::DrawProperties&, CtmQueue&, SGBitSet*) const;

         virtual void      info(std::ostringstream&, real) const;
         virtual void      write(OutputTdtFile* const tedfile) const;
         virtual void      dbExport(DbExportFile&) const;
//         virtual word      numPoints() const {return _psize;}
         virtual bool      pointInside(const TP)const;
         virtual PointVector dumpPoints() const;
         virtual laydata::ShapeList* getRepaired() const;
      private:
         float             get_distance(const TP&, const TP&, const TP&)const;
         int4b*            _pdata;
         unsigned          _psize;
         WireWidth         _width;
   };

//   typedef laydata::QTreeTmpl<TdtAuxData>       QuadTree;
//   typedef laydata::QTStoreTmpl<TdtAuxData>     QTreeTmp;
//   typedef laydata::LayerContainer<QuadTree*>   LayerHolder;
   typedef std::list<GrcData*>               AuxDataList;
//   typedef  std::map<unsigned, QTreeTmp*>       TmpLayerMap;


   class GrcCell {
      public:
                             GrcCell(std::string);
                             GrcCell(InputTdtFile* const, std::string);
         virtual            ~GrcCell();
         virtual void        write(OutputTdtFile* const) const;
         void                dbExport(DbExportFile&) const;
         virtual void        openGlRender(trend::TrendBase&, const CTM&, bool, bool) const;
         virtual DBbox       getVisibleOverlap(const layprop::DrawProperties&);
         virtual void        collectUsedLays(LayerDefList&) const;
         virtual void        motionDraw(trend::TrendBase&/*, CtmQueue&, bool active=false*/) const;
         //
         QuadTreeGrc*        secureLayer(const LayerDef&);
         QTreeTmpGrc*        secureUnsortedLayer(const LayerDef&);
         bool                fixUnsorted();
         //
         void                reportLayers(LayerDefSet&);
         void                reportLayData(const LayerDef&, AuxDataList&);
         char                cleanLay(const LayerDef&, AuxDataList&);
         bool                repairData(const LayerDef&, laydata::ShapeList&);
         char                cleanRepaired(const LayerDef&, AuxDataList& recovered);

         //
         virtual DBbox       cellOverlap() const        {return _cellOverlap;}
         std::string         name() const               {return _name;}
      protected:
         void                readTdtLay(InputTdtFile* const);
         void                getCellOverlap();
         std::string         _name;         //! cell name
         LayerHolderGrc      _layers;       //! all layers in the cell
         DBbox               _cellOverlap;  //! Overlap of the entire cell
         TmpLayerMapGrc      _tmpLayers;    //! All layers with unsorted data
   };

}
#endif /* AUXDAT_H_ */
