#include <wx/app.h>

#include "Tetris.hpp"

class TetrisApp : public wxApp
{
public:
  virtual bool OnInit() override
  {
    auto pTetris = new wxx::TetrisFrame;
    pTetris->Center();
    pTetris->Show();
    return true;
  }
};

IMPLEMENT_APP(TetrisApp)
