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
//        Created: Sun Mar 17 2002
//     Originator: Svilen Krustev - skr@toped.org.uk
//    Description: Toped prompt
//---------------------------------------------------------------------------
//  Revision info
//---------------------------------------------------------------------------
//      $Revision$
//          $Date$
//        $Author$
//===========================================================================

#ifndef TED_PROMPT_H
#define TED_PROMPT_H

#include <wx/textctrl.h>
#include <wx/thread.h>
#include <wx/string.h>
#include "tellyzer.h"
#include "outbox.h"

namespace console {

   typedef std::list<std::string>   stringList;
   const wxString real_tmpl      = wxT("[-+]?([[:digit:]]+(\\.[[:digit:]]*)?|(\\.[[:digit:]]+))");
   const wxString bool_tmpl      = wxT("true|false");
   const wxString point_tmpl     = wxT("\\{")+ real_tmpl+wxT(",")+ real_tmpl+wxT("\\}");
   const wxString box_tmpl       = wxT("\\{")+point_tmpl+wxT(",")+point_tmpl+wxT("\\}");
   const wxString bind_tmpl      = wxT("\\{")+point_tmpl+wxT(",")+real_tmpl+wxT(",")+bool_tmpl+wxT(",")+real_tmpl+wxT("\\}");
   const wxString pointlist_tmpl = wxT("\\{")+point_tmpl+wxT("(,")+point_tmpl+wxT("){1,}\\}");

   bool patternFound(const wxString templ,  wxString str);
   void patternNormalize(wxString& str);

   class miniParser {
   public:
                              miniParser(telldata::operandSTACK *,telldata::typeID);
      bool                    getGUInput(wxString expression);
      telldata::typeID        wait4type() { return _wait4type;};
   private:
      bool                    getPoint();
      bool                    getBox();
      bool                    getList();
      bool                    getBind();
      telldata::operandSTACK *client_stack;
      telldata::typeID         _wait4type;
      wxString                  exp;
   };

   class ted_cmd : public wxTextCtrl {
   public:
                              ted_cmd(wxWindow*, wxWindow*);
      virtual                ~ted_cmd();
      void                    parseCommand(wxString, bool thread=true);
      void                    waitGUInput(telldata::operandSTACK*,console::ACTIVE_OP, const CTM&);
      void                    getGUInput(bool from_keyboard = true);
      wxCondition*            threadWaits4;
      miniParser*             puc; // parse user coordinates

      void                    getCommand(bool);
      bool                    mouseIN_OK() const            {return _mouseIN_OK;};
      word                    numpoints() const             {return _numpoints;}
      const char*             lastCommand() const           {return _cmd_history.rbegin()->c_str();}
      void                    set_canvas_invalid(bool val)  { _canvas_invalid = val;}
      bool                    canvas_invalid()              {return _canvas_invalid;}
      void                    setExitRequest(bool val)      { _exitRequested = val;}
      bool                    exitRequested() const         { return _exitRequested;}
      bool                    cmdHistoryExists() const      {return !_cmd_history.empty();}
      void                    spawnParseThread(wxString);
   private:
      void                    onParseCommand(wxCommandEvent&);
      void                    onGetCommand(wxCommandEvent& WXUNUSED(event));
      void                    onKeyUP(wxKeyEvent&);
      void                    onGUInput(wxCommandEvent&);
      void                    cancelLastPoint();
      void                    mouseLB(const telldata::ttpnt& p);
      void                    mouseRB();
      word                    _numpoints;
      CTM                     _translation;
      CTM                     _initrans;
      bool                    _mouseIN_OK;
      wxString                _guinput;
      stringList              _cmd_history;
      stringList::const_iterator _history_position;
      wxWindow*               _canvas;
      bool                    _canvas_invalid;
      bool                    _exitRequested;
      DECLARE_EVENT_TABLE();
   };

   class parse_thread : public wxThread
   {
   public:
      parse_thread(wxString& cmd, wxWindow* canvas_wnd, wxThreadKind kind=wxTHREAD_DETACHED):
                  wxThread(kind),command(cmd), _canvas_wnd(canvas_wnd){};
      friend ted_cmd::ted_cmd( wxWindow*, wxWindow*);
      friend void ted_cmd::spawnParseThread(wxString);
   protected:
      void*                   Entry();
      wxString                command;
      wxWindow*               _canvas_wnd;
      static wxMutex          _mutex;
   };
}
#endif
