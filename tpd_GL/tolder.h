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
//   This file is a part of Toped project (C) 2001-2009 Toped developers    =
// ------------------------------------------------------------------------ =
//           $URL$
//        Created: Sun Sep 16 BST 2012
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: OpenGL Basic renderer
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================
#ifndef TOLDER_H
#define TOLDER_H

#include <GL/glew.h>
#include "basetrend.h"

namespace trend {

   class TolderTV : public TrendTV {
      public:
                           TolderTV(TrendRef* const, bool, bool, unsigned, unsigned);
         virtual          ~TolderTV();

         virtual void      collect(TNDR_GLDATAT*, unsigned int*);
         virtual void      draw(layprop::DrawProperties*);
         virtual void      drawTexts(layprop::DrawProperties*);
   };

   class TolderReTV : public TrendReTV {
      public:
                           TolderReTV(TrendTV* const chunk, TrendRef* const refCell):
                              TrendReTV(chunk, refCell) {}
         virtual void      draw(layprop::DrawProperties*);
         virtual void      drawTexts(layprop::DrawProperties*);
   };

   class TolderLay : public TrendLay {
      public:
                           TolderLay();
         virtual          ~TolderLay();
         virtual void      newSlice(TrendRef* const, bool, bool /*, bool, unsigned*/);
         virtual void      newSlice(TrendRef* const, bool, bool, unsigned slctd_array_offset);
         virtual bool      chunkExists(TrendRef* const, bool);
         virtual void      draw(layprop::DrawProperties*);
         virtual void      drawSelected();
         virtual void      drawTexts(layprop::DrawProperties*);
         virtual void      collect(bool, GLuint, GLuint);
         virtual void      collectSelected(unsigned int*);
   };

   class TolderRefLay : public TrendRefLay {
      public:
                           TolderRefLay();
         virtual          ~TolderRefLay();
         virtual void      collect(GLuint);
         virtual void      draw(layprop::DrawProperties*);
   };

   class Tolder : public TrendBase {
      public:
                           Tolder( layprop::DrawProperties* drawprop, real UU );
         virtual          ~Tolder();
         virtual void      grid( const real, const std::string );
         virtual void      setLayer(const LayerDef&, bool);
         virtual void      setGrcLayer(bool, const LayerDef&);
         virtual bool      chunkExists(const LayerDef&, bool);
         virtual bool      collect();
         virtual bool      grcCollect();
         virtual void      draw();
         virtual void      grcDraw();
         virtual void      cleanUp();
         virtual void      grcCleanUp();
      private:
         unsigned int*     _sindex_array;
//         unsigned          _num_ogl_buffers; //! Number of generated openGL VBOs
//         unsigned          _num_ogl_grc_buffers; //!
//         GLuint*           _ogl_buffers;     //! Array with the "names" of all openGL buffers
//         GLuint*           _ogl_grc_buffers; //! Array with the "names" of the GRC related openGL buffers
//         GLuint            _sbuffer;         //! The "name" of the selected index buffer
   };

}

#endif //TOLDER_H
