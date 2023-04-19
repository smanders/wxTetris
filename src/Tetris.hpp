#ifndef TETRIS_HPP
#define TETRIS_HPP

#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/panel.h>

namespace wxx
{
  // In your wxApp-derived class, overridden OnInit method:
  //  wxx::TetrisFrame* pTetris = new wxx::TetrisFrame();
  //  pTetris->Center();
  //  pTetris->Show();
  //  return true;

  class TetrisFrame : public wxFrame
  {
  public:
    TetrisFrame(const wxString& rTitle = wxT("Tetris"));
  };

  // In an event-handler:
  // Connect(wxEVT_RIGHT_DCLICK, wxMouseEventHandler(foo::onRightDClick));
  // void foo::onRightDClick(wxMouseEvent& rEvent)
  // {
  //   if (rEvent.ControlDown() && rEvent.AltDown())
  //   {
  //     wxx::TetrisDialog* pTetris(new wxx::TetrisDialog(this));
  //     pTetris->Show();
  //   }
  // }

  class TetrisDialog : public wxDialog
  {
  public:
    TetrisDialog(wxWindow* pParent, const wxString& rTitle = wxT("Tetris"));
  };

  class Board;
  class TetrisPanel : public wxPanel
  {
  public:
    TetrisPanel(wxTopLevelWindow* pParent);
    virtual bool Show(bool show = true);

  private:
    wxStatusBar* m_pStatus;
    Board* m_pBoard;
  };
} // namespace wxx

class FunEvent : public wxCommandEvent
{
public:
  FunEvent(wxEventType commandType = wxEVT_NULL,
           int winid = 0,
           bool show = false)
    : wxCommandEvent(commandType, winid), m_show(show)
  {
  }
  FunEvent(const FunEvent& evt) : wxCommandEvent(evt), m_show(false) { }
  virtual wxEvent* Clone() const { return new FunEvent(*this); }
  bool show() const { return m_show; }

private:
  const bool m_show;
};
typedef void (wxEvtHandler::*FunEventFunction)(FunEvent&);
#define FunEventHandler(func)                                                  \
  (wxObjectEventFunction)(wxEventFunction)                                     \
    wxStaticCastEvent(FunEventFunction, &func)

#endif // TETRIS_HPP
