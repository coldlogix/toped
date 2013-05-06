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
//        Created: Sun Oct 14 BST 2012
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: OpenGL shader renderer
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#include "tpdph.h"
#include "toshader.h"
#include "viewprop.h"
#include "trend.h"

extern trend::TrendCenter*         TRENDC;

       
void trend::setShaderCtm(layprop::DrawProperties* drawprop, const TrxCellRef* refCell)
{
   drawprop->pushCtm(refCell->ctm() * drawprop->topCtm());
   float mtrxOrtho [16];
   drawprop->topCtm().oglForm(mtrxOrtho);
   glUniformMatrix4fv(TRENDC->getUniformLoc(glslu_in_CTM), 1, GL_FALSE, mtrxOrtho);
}
       
//=============================================================================
//
// class ToshaderTV
//
trend::ToshaderTV::ToshaderTV(TrxCellRef* const refCell, bool filled, bool reusable,
                                    unsigned parray_offset, unsigned iarray_offset) :
   TenderTV(refCell, filled, reusable, parray_offset, iarray_offset)
{}

void trend::ToshaderTV::draw(layprop::DrawProperties* drawprop)
{
   // First - deal with openGL translation matrix
   setShaderCtm(drawprop, _refCell);
   setAlpha(drawprop);
   // Activate the vertex buffers in the vertex shader ...
   glEnableVertexAttribArray(TSHDR_LOC_VERTEX);  // glEnableClientState(GL_VERTEX_ARRAY)
   // Set-up the offset in the binded Vertex buffer
   glVertexAttribPointer(TSHDR_LOC_VERTEX, 2, TNDR_GLENUMT, GL_FALSE, 0, (GLvoid*)(sizeof(TNDR_GLDATAT) * _point_array_offset));
   // ... and here we go ...
   drawTriQuads();
   glUniform1ui(TRENDC->getUniformLoc(glslu_in_StippleEn), 0);
   drawLines();
   glUniform1ui(TRENDC->getUniformLoc(glslu_in_StippleEn), 1);
   // Switch the vertex buffers OFF in the openGL engine ...
   glDisableVertexAttribArray(TSHDR_LOC_VERTEX);
   // ... and finally restore the openGL translation matrix
   drawprop->popCtm();
}

void trend::ToshaderTV::drawTexts(layprop::DrawProperties* drawprop)
{
   setShaderCtm(drawprop, _refCell);
   setAlpha(drawprop);
   for (TrendStrings::const_iterator TSTR = _text_data.begin(); TSTR != _text_data.end(); TSTR++)
   {
      //  Things to remember...
      // Font has to be translated using its own matrix in which
      // tx/ty are forced to zero. 
      // The text binding point (i.e. tx/ty) is multiplied ALONE with the current
      // translation matrix, but NEVER with the font matrix.
      CTM ftm((*TSTR)->ctm());
      CTM ctm( ftm.a(), ftm.b(), ftm.c(), ftm.d(), 0,0);
      ctm.Scale((real)OPENGL_FONT_UNIT, (real)OPENGL_FONT_UNIT);
      ctm.Translate(TP(ftm.tx(), ftm.ty()));
      drawprop->pushCtm(ctm * drawprop->topCtm());
      float mtrxOrtho[16];
      drawprop->topCtm().oglForm(mtrxOrtho);
      glUniformMatrix4fv(TRENDC->getUniformLoc(glslu_in_CTM), 1, GL_FALSE, mtrxOrtho);
      (*TSTR)->draw(_filled, drawprop);
      drawprop->popCtm();
   }
   drawprop->popCtm();
}

void trend::ToshaderTV::drawTriQuads()
{
   if  (_alobjvx[cnvx] > 0)
   {// Draw convex polygons
      assert(_firstvx[cnvx]);
      assert(_sizesvx[cnvx]);
      glMultiDrawArrays(GL_QUADS, _firstvx[cnvx], _sizesvx[cnvx], _alobjvx[cnvx]);
   }
   if  (_alobjvx[ncvx] > 0)
   {// Draw non-convex polygons
      if (_alobjix[fqss] > 0)
      {
         assert(_sizesix[fqss]);
         assert(_firstix[fqss]);
         // The line below works on Windows, but doesn't work (hangs) on Linux with nVidia driver.
         // The suspect is (const GLvoid**)_firstix[fqss] but it's quite possible that it is a driver bug
         // Besides - everybody is saying that there is no speed benefit from this operation
         //glMultiDrawElements(GL_QUAD_STRIP    , _sizesix[fqss], GL_UNSIGNED_INT, (const GLvoid**)_firstix[fqss], _alobjix[fqss]);
         for (unsigned i= 0; i < _alobjix[fqss]; i++)
            glDrawElements(GL_QUAD_STRIP, _sizesix[fqss][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_firstix[fqss][i]));
      }
      if (_alobjix[ftrs] > 0)
      {
         assert(_sizesix[ftrs]);
         assert(_firstix[ftrs]);
         //glMultiDrawElements(GL_TRIANGLES     , _sizesix[ftrs], GL_UNSIGNED_INT, (const GLvoid**)_firstix[ftrs], _alobjix[ftrs]);
         for (unsigned i= 0; i < _alobjix[ftrs]; i++)
            glDrawElements(GL_TRIANGLES, _sizesix[ftrs][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_firstix[ftrs][i]));
      }
      if (_alobjix[ftfs] > 0)
      {
         assert(_sizesix[ftfs]);
         assert(_firstix[ftfs]);
         //glMultiDrawElements(GL_TRIANGLE_FAN  , _sizesix[ftfs], GL_UNSIGNED_INT, (const GLvoid**)_firstix[ftfs], _alobjix[ftfs]);
         for (unsigned i= 0; i < _alobjix[ftfs]; i++)
            glDrawElements(GL_TRIANGLE_FAN, _sizesix[ftfs][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_firstix[ftfs][i]));
      }
      if (_alobjix[ftss] > 0)
      {
         assert(_sizesix[ftss]);
         assert(_firstix[ftss]);
         //glMultiDrawElements(GL_TRIANGLE_STRIP, _sizesix[ftss], GL_UNSIGNED_INT, (const GLvoid**)_firstix[ftss], _alobjix[ftss]);
         for (unsigned i= 0; i < _alobjix[ftss]; i++)
            glDrawElements(GL_TRIANGLE_STRIP, _sizesix[ftss][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_firstix[ftss][i]));
      }
   }
}

void trend::ToshaderTV::drawLines()
{
   if  (_alobjvx[line] > 0)
   {// Draw the wire center lines
      assert(_firstvx[line]);
      assert(_sizesvx[line]);
      glMultiDrawArrays(GL_LINE_STRIP, _firstvx[line], _sizesvx[line], _alobjvx[line]);
   }
   if  (_alobjvx[cnvx] > 0)
   {// Draw convex polygons
      assert(_firstvx[cnvx]);
      assert(_sizesvx[cnvx]);
      glMultiDrawArrays(GL_LINE_LOOP, _firstvx[cnvx], _sizesvx[cnvx], _alobjvx[cnvx]);
   }
   if  (_alobjvx[ncvx] > 0)
   {// Draw non-convex polygons
      glEnableClientState(GL_INDEX_ARRAY);
      assert(_firstvx[ncvx]);
      assert(_sizesvx[ncvx]);
      glMultiDrawArrays(GL_LINE_LOOP, _firstvx[ncvx], _sizesvx[ncvx], _alobjvx[ncvx]);
   }
   if (_alobjvx[cont] > 0)
   {// Draw the remaining non-filled shapes of any kind
      assert(_firstvx[cont]);
      assert(_sizesvx[cont]);
      glMultiDrawArrays(GL_LINE_LOOP, _firstvx[cont], _sizesvx[cont], _alobjvx[cont]);
   }
}

void trend::ToshaderTV::setAlpha(layprop::DrawProperties* drawprop)
{
   layprop::tellRGB tellColor;
   if (drawprop->getAlpha(_refCell->alphaDepth() - 1, tellColor))
   {
      float alpha = (float)tellColor.alpha() / 255.0f;
      glUniform1f(TRENDC->getUniformLoc(glslu_in_Alpha), alpha);
   }
}

//=============================================================================
//
// class ToshaderReTV
//
//void trend::ToshaderReTV::draw(layprop::DrawProperties* drawprop)
//{
//   TrxCellRef* sref_cell = _chunk->swapRefCells(_refCell);
//   _chunk->draw(drawprop);
//   _chunk->swapRefCells(sref_cell);
//}
//
//void trend::ToshaderReTV::drawTexts(layprop::DrawProperties* drawprop)
//{
//   TrxCellRef* sref_cell = _chunk->swapRefCells(_refCell);
//   _chunk->drawTexts(drawprop);
//   _chunk->swapRefCells(sref_cell);
//}


//=============================================================================
//
// class ToshaderLay
//
trend::ToshaderLay::ToshaderLay():
   TenderLay           (             )
{
}

/**
 * Create a new object of TenderTV type which will be referred to by _cslice
 * @param ctrans Current translation matrix of the new object
 * @param fill Whether to fill the drawing objects
 */
void trend::ToshaderLay::newSlice(TrxCellRef* const ctrans, bool fill, bool reusable, unsigned slctd_array_offset)
{
   assert( 0 == total_slctdx());
   _slctd_array_offset = slctd_array_offset;
   _stv_array_offset = 2 * _num_total_points;
   newSlice(ctrans, fill, reusable);
}

void trend::ToshaderLay::newSlice(TrxCellRef* const ctrans, bool fill, bool reusable)
{
   _cslice = DEBUG_NEW ToshaderTV(ctrans, fill, reusable, 2 * _num_total_points, _num_total_indexs);
}

bool trend::ToshaderLay::chunkExists(TrxCellRef* const ctrans, bool filled)
{
   ReusableTTVMap::iterator achunk;
   if (filled)
   {
      if (_reusableFData.end() == ( achunk =_reusableFData.find(ctrans->name()) ) )
         return false;
   }
   else
   {
      if (_reusableCData.end() == ( achunk =_reusableCData.find(ctrans->name()) ) )
         return false;
   }
   _reLayData.push_back(DEBUG_NEW ToshaderReTV(achunk->second, ctrans));
   return true;
}

void trend::ToshaderLay::drawSelected()
{
   glBindBuffer(GL_ARRAY_BUFFER, _pbuffer);
   // Check the state of the buffer
   GLint bufferSize;
   glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
   assert(bufferSize == (GLint)(2 * _num_total_points * sizeof(TNDR_GLDATAT)));

   // Activate the vertex buffers in the vertex shader ...
   glEnableVertexAttribArray(TSHDR_LOC_VERTEX);  // glEnableClientState(GL_VERTEX_ARRAY)
   // Set-up the offset in the binded Vertex buffer
   glVertexAttribPointer(TSHDR_LOC_VERTEX, 2, TNDR_GLENUMT, GL_FALSE, 0, (GLvoid*)(sizeof(TNDR_GLDATAT) * _stv_array_offset));

   if (_asobjix[lstr] > 0)
   {
      assert(_sizslix[lstr]);
      assert(_fstslix[lstr]);
      //glMultiDrawElements(GL_LINE_STRIP, _sizslix[lstr], GL_UNSIGNED_INT, (const GLvoid**)_fstslix[lstr], _asobjix[lstr]);
      for (unsigned i= 0; i < _asobjix[lstr]; i++)
         glDrawElements(GL_LINE_STRIP, _sizslix[lstr][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_fstslix[lstr][i]));
   }
   if (_asobjix[llps] > 0)
   {
      assert(_sizslix[llps]);
      assert(_fstslix[llps]);
         //glMultiDrawElements(GL_LINE_LOOP     , _sizslix[llps], GL_UNSIGNED_INT, (const GLvoid**)_fstslix[llps], _alobjix[llps]);
      for (unsigned i= 0; i < _asobjix[llps]; i++)
         glDrawElements(GL_LINE_LOOP, _sizslix[llps][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_fstslix[llps][i]));
   }
   if (_asobjix[lnes] > 0)
   {
      assert(_sizslix[lnes]);
      assert(_fstslix[lnes]);
         //glMultiDrawElements(GL_LINES  , _sizslix[lnes], GL_UNSIGNED_INT, (const GLvoid**)_fstslix[lnes], _alobjix[lnes]);
      for (unsigned i= 0; i < _asobjix[lnes]; i++)
         glDrawElements(GL_LINES, _sizslix[lnes][i], GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(_fstslix[lnes][i]));
   }
   glDisableVertexAttribArray(TSHDR_LOC_VERTEX);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}


//=============================================================================
//
// class TenderRefLay
//
trend::ToshaderRefLay::ToshaderRefLay() :
   TenderRefLay    (      )
{
}


void trend::ToshaderRefLay::draw(layprop::DrawProperties* drawprop)
{
   // Bind the buffer
   glBindBuffer(GL_ARRAY_BUFFER, _pbuffer);
   // Check the state of the buffer
   GLint bufferSize;
   glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
   assert(bufferSize == (GLint)(2 * total_points() * sizeof(TNDR_GLDATAT)));

   glEnableVertexAttribArray(TSHDR_LOC_VERTEX);
   // Set-up the offset in the binded Vertex buffer
   glVertexAttribPointer(TSHDR_LOC_VERTEX, 2, TNDR_GLENUMT, GL_FALSE, 0, 0);
   // ... and here we go ...
   if (0 < (_alvrtxs + _asindxs))
   {
      assert(_firstvx); assert(_sizesvx);
      glMultiDrawArrays(GL_LINE_LOOP, _firstvx, _sizesvx, _alobjvx + _asobjix);
      if (0 < _asindxs)
      {
         assert(_fstslix); assert(_sizslix);
         setLine(drawprop, true);
         glMultiDrawArrays(GL_LINE_LOOP, _fstslix, _sizslix, _asobjix);
         setLine(drawprop, false);
      }
   }
   glDisableVertexAttribArray(TSHDR_LOC_VERTEX);
}

void trend::ToshaderRefLay::setLine(layprop::DrawProperties* drawprop, bool selected)
{
   layprop::LineSettings curLine;
   drawprop->getCurrentLine(curLine, selected);
   glLineWidth(curLine.width());
   if (0xffff == curLine.pattern())
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_LStippleEn), 0);
   else
   {
      GLuint lPattern = curLine.pattern();
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_LStipple), lPattern);
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_PatScale), curLine.patscale());
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_LStippleEn), 1);
   }
}

trend::ToshaderRefLay::~ToshaderRefLay()
{
}

//=============================================================================
//
// class ToshaderMarks
//
void trend::ToshaderMarks::draw(layprop::DrawProperties* drawprop)
{
   // Bind the buffer
   glBindBuffer(GL_ARRAY_BUFFER, _pbuffer);
   // Check the state of the buffer
   GLint bufferSize;
   glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
   assert(bufferSize == (GLint)(2 * total_points() * sizeof(TNDR_GLDATAT)));

   glEnableVertexAttribArray(TSHDR_LOC_VERTEX);
   // Set-up the offset in the binded Vertex buffer
   glVertexAttribPointer(TSHDR_LOC_VERTEX, 2, TNDR_GLENUMT, GL_FALSE, 0, 0);
   unsigned start = 0;
   unsigned size = _refMarks.size();
   if (0 < size)
   {
      setStipple(drawprop->ref_mark_bmp());
      glDrawArrays(GL_POINTS, start, size);
      start += size;
   }
   if (0 < (size = _textMarks.size()) )
   {
      setStipple(drawprop->text_mark_bmp());
      glDrawArrays(GL_POINTS, start, size);
      start += size;
   }
   if (0 < (size = _arefMarks.size()) )
   {
      setStipple(drawprop->aref_mark_bmp());
      glDrawArrays(GL_POINTS, start, size);
   }
   glDisableVertexAttribArray(TSHDR_LOC_VERTEX);

}

void trend::ToshaderMarks::setStipple(const byte* markStipple)
{
   // the matrix above must be converted to something suitable by the shader
   // the size of the array below should be 32 of course. The AMD platform however
   // behaves in a weird way, so the simplest workaround is to to expand the
   // array with one and to bind the first one to 0. The bug was reported here:
   // http://devgurus.amd.com/thread/160053
   // Note that the fragment shader is updated accordingly.
   GLuint shdrStipple [33];
   shdrStipple[0] = 0;
   for (unsigned i = 0; i < 16; i++)
      shdrStipple[i+1] = ((GLuint)(markStipple[2*i + 0]) << 8*1)
                       | ((GLuint)(markStipple[2*i + 1]) << 8*0);
   for (unsigned i = 16; i < 32; i++)
      shdrStipple[i+1] = 0;
   glUniform1uiv(TRENDC->getUniformLoc(glslu_in_Stipple), 33, shdrStipple);
}

//=============================================================================
//
// class Toshader
//
trend::Toshader::Toshader( layprop::DrawProperties* drawprop, real UU) :
    Tenderer             (drawprop, UU, false )
{
   _refLayer = DEBUG_NEW ToshaderRefLay();
   _marks    = DEBUG_NEW ToshaderMarks();
}

bool trend::Toshader::chunkExists(const LayerDef& laydef, bool has_selected)
{
   // Reference layer is processed differently (pushCell), so make sure
   // that we haven't got here with REF_LAY by accident
   assert(REF_LAY_DEF != laydef);
   if (NULL != _clayer)
   { // post process the current layer
      _clayer->ppSlice();
      _cslctd_array_offset += _clayer->total_slctdx();
   }
   if (_data.end() != _data.find(laydef))
   {
      _clayer = _data[laydef];
      if (_clayer->chunkExists(_cellStack.top(), _drawprop->layerFilled(laydef) ) ) return true;
   }
   else
   {
      _clayer = DEBUG_NEW ToshaderLay();
      _data.add(laydef, _clayer);
   }
   if (has_selected)
      _clayer->newSlice(_cellStack.top(), _drawprop->layerFilled(laydef), true, _cslctd_array_offset);
   else
      _clayer->newSlice(_cellStack.top(), _drawprop->layerFilled(laydef), true);
   return false;
}

void trend::Toshader::setLayer(const LayerDef& laydef, bool has_selected)
{
   // Reference layer is processed differently (pushCell), so make sure
   // that we haven't got here with REF_LAY by accident
   assert(REF_LAY_DEF != laydef);
   if (NULL != _clayer)
   { // post process the current layer
      _clayer->ppSlice();
      _cslctd_array_offset += _clayer->total_slctdx();
   }
   if (_data.end() != _data.find(laydef))
   {
      _clayer = _data[laydef];
   }
   else
   {
      _clayer = DEBUG_NEW ToshaderLay();
      _data.add(laydef, _clayer);
   }
   if (has_selected)
      _clayer->newSlice(_cellStack.top(), _drawprop->layerFilled(laydef), false, _cslctd_array_offset);
   else
      _clayer->newSlice(_cellStack.top(), _drawprop->layerFilled(laydef), false);
}

void trend::Toshader::setHvrLayer(const LayerDef& laydef)
{
   if (REF_LAY_DEF != laydef)
   {
      _clayer = DEBUG_NEW ToshaderLay();
      _data.add(laydef, _clayer);
      _clayer->newSlice(_cellStack.top(), false, false, 0 /*_cslctd_array_offset*/);
   }
}

void trend::Toshader::grdDraw()
{
   TRENDC->setGlslProg(glslp_VF);
   glUniform1ui(TRENDC->getUniformLoc(glslu_in_StippleEn), 0);

   _drawprop->initCtmStack();
   float mtrxOrtho [16];
   _drawprop->topCtm().oglForm(mtrxOrtho);
   glUniformMatrix4fv(TRENDC->getUniformLoc(glslu_in_CTM), 1, GL_FALSE, mtrxOrtho);

   glEnableVertexAttribArray(TSHDR_LOC_VERTEX);

   glBindBuffer(GL_ARRAY_BUFFER, _ogl_grd_buffer[0]);
   unsigned startP = 0;
   for (VGrids::const_iterator CG = _grid_props.begin(); CG != _grid_props.end(); CG++)
   {
      // color
      layprop::tellRGB theColor(_drawprop->getColor((*CG)->color()));
      float oglColor[4];
      oglColor[0] = (float)theColor.red()   / 255.0f ;
      oglColor[1] = (float)theColor.green() / 255.0f ;
      oglColor[2] = (float)theColor.blue()  / 255.0f ;
      oglColor[3] = (float)theColor.alpha() / 255.0f ;
      glUniform3fv(TRENDC->getUniformLoc(glslu_in_Color), 1, oglColor);
      glUniform1f(TRENDC->getUniformLoc(glslu_in_Alpha), oglColor[3]);
      //draw
      glVertexAttribPointer(TSHDR_LOC_VERTEX, 2, TNDR_GLENUMT, GL_FALSE, 0, 0);
      glDrawArrays(GL_POINTS, startP, (*CG)->asize());
      startP += (*CG)->asize();
   }
   glDisableVertexAttribArray(TSHDR_LOC_VERTEX);
   // clean-up the buffers
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void trend::Toshader::setLayColor(const LayerDef& layer)
{
   layprop::tellRGB tellColor;
   if (_drawprop->setCurrentColor(layer, tellColor))
   {
      float oglColor[4];
      oglColor[0] = (float)tellColor.red()   / 255.0f ;
      oglColor[1] = (float)tellColor.green() / 255.0f ;
      oglColor[2] = (float)tellColor.blue()  / 255.0f ;
      oglColor[3] = (float)tellColor.alpha() / 255.0f ;
      glUniform3fv(TRENDC->getUniformLoc(glslu_in_Color), 1, oglColor);
      glUniform1f(TRENDC->getUniformLoc(glslu_in_Alpha), oglColor[3]);
   }
}

void trend::Toshader::setStipple()
{
   const byte* tellStipple = _drawprop->getCurrentFill();
   if (NULL == tellStipple)
   {
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_StippleEn), 0);
   }
   else
   {
      // the matrix above must be converted to something suitable by the shader
      // the size of the array below should be 32 of course. The AMD platform however
      // behaves in a weird way, so the simplest workaround is to to expand the
      // array with one and to bind the first one to 0. The bug was reported here:
      // http://devgurus.amd.com/thread/160053
      // Note that the fragment shader is updated accordingly.
      GLuint shdrStipple [33];
      shdrStipple[0] = 0;
      for (unsigned i = 0; i < 32; i++)
         shdrStipple[i+1] = ((GLuint)(tellStipple[4*i + 0]) << 8*3)
                          | ((GLuint)(tellStipple[4*i + 1]) << 8*2)
                          | ((GLuint)(tellStipple[4*i + 2]) << 8*1)
                          | ((GLuint)(tellStipple[4*i + 3]) << 8*0);
      glUniform1uiv(TRENDC->getUniformLoc(glslu_in_Stipple), 33, shdrStipple);
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_StippleEn), 1);
   }
}

void trend::Toshader::setLine(bool selected)
{
   layprop::LineSettings curLine;
   _drawprop->getCurrentLine(curLine, selected);
   glLineWidth(curLine.width());
   if (0xffff == curLine.pattern())
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_LStippleEn), 0);
   else
   {
      GLuint lPattern = curLine.pattern();
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_LStipple), lPattern);
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_PatScale), curLine.patscale());
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_LStippleEn), 1);
   }
}

void trend::Toshader::draw()
{
   _drawprop->initCtmStack();
   _drawprop->resetCurrentColor();
   TRENDC->setGlslProg(glslp_VF);
   for (DataLay::Iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
   {// for every layer
      setLayColor(CLAY());
      setLine(false);
      setStipple();
      // draw everything
      if (0 != CLAY->total_points())
         CLAY->draw(_drawprop);
      // draw texts
      if (0 != CLAY->total_strings())
      {
         TRENDC->bindFont();
         CLAY->drawTexts(_drawprop);
      }
   }
   // Lines with stipples
   TRENDC->setGlslProg(glslp_VG);
   glUniform1ui(TRENDC->getUniformLoc(glslu_in_StippleEn), 0);
   for (DataLay::Iterator CLAY = _data.begin(); CLAY != _data.end(); CLAY++)
   {// for every layer
      if (0 != CLAY->total_slctdx())
      {// redraw selected contours only
         setLayColor(CLAY());
         setLine(true);
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _sbuffer);
         setShaderCtm(_drawprop, _activeCS);
         CLAY->drawSelected();
         _drawprop->popCtm();
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      }
   }
   setLayColor(REF_LAY_DEF);
   // draw reference boxes
   if (0 < _refLayer->total_points())
   {
      setLine(false);
      float mtrxOrtho [16];
      _drawprop->topCtm().oglForm(mtrxOrtho);
      glUniformMatrix4fv(TRENDC->getUniformLoc(glslu_in_CTM), 1, GL_FALSE, mtrxOrtho);
      _refLayer->draw(_drawprop);
   }
   // draw reference marks
   if (0 < _marks->total_points())
   {
      TRENDC->setGlslProg(glslp_PS);
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_StippleEn) , 0);
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_LStippleEn), 0);
      glUniform1ui(TRENDC->getUniformLoc(glslu_in_MStippleEn), 1);
      setLayColor(REF_LAY_DEF);
      float mtrxOrtho [16];
      _drawprop->topCtm().oglForm(mtrxOrtho);
      glUniformMatrix4fv(TRENDC->getUniformLoc(glslu_in_CTM), 1, GL_FALSE, mtrxOrtho);
      _marks->draw(_drawprop);
   }

   checkOGLError("draw");
   _drawprop->clearCtmStack();
}

void trend::Toshader::grcDraw()
{
   //TODO
}

void trend::Toshader::rlrDraw()
{
   TRENDC->setGlslProg(glslp_VF);
   glUniform1ui(TRENDC->getUniformLoc(glslu_in_StippleEn), 0);

   _drawprop->initCtmStack();
   float mtrxOrtho [16];
   _drawprop->topCtm().oglForm(mtrxOrtho);
   glUniformMatrix4fv(TRENDC->getUniformLoc(glslu_in_CTM), 1, GL_FALSE, mtrxOrtho);

   glEnableVertexAttribArray(TSHDR_LOC_VERTEX);

   // color
   float oglColor[4];
   oglColor[0] = 1.0f ;
   oglColor[1] = 1.0f ;
   oglColor[2] = 1.0f ;
   oglColor[3] = 0.7f ;
   glUniform3fv(TRENDC->getUniformLoc(glslu_in_Color), 1, oglColor);
   glUniform1f(TRENDC->getUniformLoc(glslu_in_Alpha), oglColor[3]);

   //draw
   glBindBuffer(GL_ARRAY_BUFFER, _ogl_rlr_buffer[0]);
   glVertexAttribPointer(TSHDR_LOC_VERTEX, 2, TNDR_GLENUMT, GL_FALSE, 0, 0);
   glDrawArrays(GL_LINES, 0, _num_ruler_ticks);

   glDisableVertexAttribArray(TSHDR_LOC_VERTEX);
   // clean-up the buffers
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // Now draw texts
//   setShaderCtm(drawprop, _refCell);
   TRENDC->bindFont();
   for (TrendStrings::const_iterator TSTR = _rulerTexts.begin(); TSTR != _rulerTexts.end(); TSTR++)
   {
      _drawprop->pushCtm((*TSTR)->ctm() * _drawprop->topCtm());
      float mtrxOrtho[16];
      _drawprop->topCtm().oglForm(mtrxOrtho);
      glUniformMatrix4fv(TRENDC->getUniformLoc(glslu_in_CTM), 1, GL_FALSE, mtrxOrtho);
      (*TSTR)->draw(false, _drawprop);
      _drawprop->popCtm();
   }
//   drawprop->popCtm();

}

void trend::Toshader::setGrcLayer(bool setEData, const LayerDef& laydef)
{
   if (setEData)
   {
      assert(_grcLayer == NULL);
      if (_grcData.end() != _grcData.find(laydef))
      {
         _grcLayer = _grcData[laydef];
      }
      else
      {
         _grcLayer = DEBUG_NEW ToshaderLay();
         _grcData.add(laydef, _grcLayer);
      }
      _grcLayer->newSlice(_cellStack.top(), false, false);
   }
   else
   {
      assert(_grcLayer != NULL);
      // post process the current layer
      _grcLayer->ppSlice();
      _grcLayer = NULL;
//      _cslctd_array_offset += _elayer->total_slctdx();
   }
}

trend::Toshader::~Toshader()
{
//   delete _refLayer; //>>> deleted in the parent constructor
}

