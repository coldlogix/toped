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
//        Created: Thu Jun 28 2012
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Database access handling (Iterators)
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
#ifndef TEDBAC_H_INCLUDED
#define TEDBAC_H_INCLUDED

#include "ttt.h"
#include "outbox.h"

namespace laydata {
//   template <typename DataT>
//   struct LayerDefMap {
//      typedef std::map<LayerDef , DataT     >     Type;
//   };


   //==========================================================================
   template <typename DataT>
   class LayerIterator {
   public:
      typedef std::map<LayerDef , DataT     > LayerDefMap;
//      typedef typename LayerDefMap<DataT>::Type LayerDefMap;
                                LayerIterator();
                                LayerIterator(const LayerDefMap*);
                                LayerIterator(const LayerDefMap*, const LayerDef&);
                                LayerIterator(const LayerIterator&);
      virtual                  ~LayerIterator();
      const LayerIterator&      operator++();    //Prefix
      const LayerIterator       operator++(int); //Postfix
      bool                      operator==(const LayerIterator&) const;
      bool                      operator!=(const LayerIterator&) const;
      DataT                     operator->() const;
      DataT                     operator*() const;
      LayerDef                  operator()() const;
      bool                      editable() const;
   protected:
      const LayerDefMap*        _layerHolder;
      typename LayerDefMap::const_iterator _cNMap;
   };

   //==========================================================================
   template <typename DataT>
   class LayerContainer {
   public:
      friend class LayerIterator<DataT>;
      typedef std::map<LayerDef , DataT     > LayerDefMap;
//      typedef typename LayerDefMap<DataT>::Type LayerDefMap;
      typedef LayerIterator<DataT> Iterator;
                                 LayerContainer();
                                 LayerContainer(const LayerContainer<DataT>&);
      virtual                   ~LayerContainer();
      const Iterator             begin() const;
      const Iterator             end() const;
      const Iterator             find(const LayerDef&) const;
      bool                       empty() const;
      size_t                     size() const;
      void                       clear();
      void                       add(const LayerDef&, DataT);
      void                       erase(const LayerDef&);
//      void                       erase(LayerIterator<DataT>);
      DataT&                     operator[](const LayerDef&);
      LayerContainer<DataT>&     operator=(const LayerContainer<DataT>&);
   private:
      LayerDefMap*               _layers;
      bool                       _copy;
   };

   //==========================================================================
   class TdtData;
   template <typename DataT>       class QTStoreTmpl;
   template <typename DataT>       class QTreeTmpl;
   typedef QTStoreTmpl<TdtData>    QTreeTmp;
   typedef QTreeTmpl<TdtData>      QuadTree;

   typedef  std::pair<TdtData*, SGBitSet>           SelectDataPair;
   typedef  std::list<SelectDataPair>               DataList;
   typedef  std::list<TdtData*>                     ShapeList;

}

namespace layprop {
   class LayerSettings;
}

namespace trend {
   class TrendLay;
}
namespace auxdata {
   class GrcData;
   class AuxData;

   typedef laydata::QTStoreTmpl<GrcData>           QTreeTmpGrc;
   typedef laydata::QTreeTmpl<GrcData>             QuadTreeGrc;
   typedef laydata::LayerContainer<QuadTreeGrc*>   LayerHolderGrc;
   typedef laydata::LayerContainer<QTreeTmpGrc*>   TmpLayerMapGrc;

   typedef laydata::QTStoreTmpl<AuxData>           QTreeTmpAux;
   typedef laydata::QTreeTmpl<AuxData>             QuadTreeAux;
//   typedef laydata::LayerContainer<QuadTreeAux*>   LayerHolderAux;
//   typedef laydata::LayerContainer<QTreeTmpAux*>   TmpLayerMapAux;
}

#endif //TEDBAC_H_INCLUDED
