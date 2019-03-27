#include "Tetris.hpp"

#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/timer.h>

namespace wxx
{
  // http://en.wikipedia.org/wiki/Tetromino
  enum Tetrominoes_e
  {
    eSHAPE_0, // no shape
    eSHAPE_I, // aka: stick, straight, long
    eSHAPE_J, // aka: inverted L, gamma
    eSHAPE_L, // aka: gun
    eSHAPE_O, // aka: square, package, block
    eSHAPE_S, // aka: inverted N
    eSHAPE_Z, // aka: N, skew, snake
    eSHAPE_T,
    eSHAPE_NUM // number of shapes
  };

  /** The Shape class saves information about the tetris piece. */
  class Shape
  {
  public:
    Shape();
    void setShape(Tetrominoes_e shape);
    void setRandomShape();
    Tetrominoes_e getShape() const;
    int x(int index) const;
    int y(int index) const;
    int minX() const;
    int maxX() const;
    int minY() const;
    int maxY() const;
    Shape rotateLeft() const;
    Shape rotateRight() const;

  private:
    void setX(int index, int x);
    void setY(int index, int y);

  private:
    Tetrominoes_e m_shape;
    int m_coords[4][2]; // saves the coordinates of the tetris piece
  };

  class Board : public wxPanel
  {
  public:
    Board(wxWindow* pParent, wxStatusBar* const pStatusBar);
    void start();

  protected: // wx event handlers
    void onPaint(wxPaintEvent& rEvent);
    void onEraseBackground(wxEraseEvent& rEvent);
    void onSize(wxSizeEvent& rEvent);
    void onKeyDown(wxKeyEvent& rEvent); // see this method for controls
    void onTimer(wxCommandEvent& rEvent);

  private:
    enum BoardDims_e
    {
      eBOARD_WIDTH = 10,
      eBOARD_HEIGHT = 22
    };
    /** if !forced, then toggle the pause state, if forced then always pause */
    void pause(bool forced = false);
    /** access the board array, returns which tetromino is at the position */
    Tetrominoes_e& shapeAt(int x, int y);
    int squareWidth();
    int squareHeight();
    void clearBoard();
    /** drops the falling shape immediately to the bottom of the board */
    void dropDown();
    /** drops the falling shape down one line */
    void oneLineDown();
    void pieceDropped();
    void removeFullLines();
    void newPiece();
    bool tryMove(const Shape& newPiece, int newX, int newY);
    void drawSquare(wxBufferedPaintDC& rDc, int x, int y, Tetrominoes_e shape);
    bool recreateBuffer(const wxSize& rSize = wxDefaultSize);

  private:
    static const int ms_timerInterval = 300;

  private:
    wxBitmap m_bufferBitmap; // used by wxBufferedPaintDC in onPaint
    wxTimer m_timer;
    bool
      m_isFallingFinished; // determines if the shape has finished falling and
                           //   if we need to create a new shape
    Shape m_curShape;
    int m_curX, m_curY; // determines the actual position of the falling shape
    int m_numLinesRemoved; // counts the number of lines removed so far (score)
    Tetrominoes_e m_board[eBOARD_WIDTH * eBOARD_HEIGHT];
    wxStatusBar* const m_pStatusBar;
  };
} // namespace wxx

using namespace wxx;

TetrisFrame::TetrisFrame(const wxString& rTitle)
  : wxFrame(NULL, wxID_ANY, rTitle, wxDefaultPosition, wxSize(384, 768))
{
  TetrisPanel* pPanel = new TetrisPanel(this);
  pPanel->Show();
}

TetrisDialog::TetrisDialog(wxWindow* pParent, const wxString& rTitle)
  : wxDialog(pParent, wxID_ANY, rTitle, wxDefaultPosition, wxSize(384, 768))
{
  TetrisPanel* pPanel = new TetrisPanel(this);
  wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);
  pSizer->Add(pPanel, 1, wxGROW);
  SetSizer(pSizer);
  pPanel->Show();
}

TetrisPanel::TetrisPanel(wxTopLevelWindow* pParent)
  : wxPanel(pParent, wxID_ANY, wxDefaultPosition, wxSize(384, 768)),
    m_pStatus(NULL),
    m_pBoard(NULL)
{
  m_pStatus = new wxStatusBar(this);
  m_pBoard = new Board(this, m_pStatus);
  wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);
  pSizer->Add(m_pBoard, 1, wxGROW);
  pSizer->Add(m_pStatus, 0, wxGROW);
  SetSizer(pSizer);
}

bool TetrisPanel::Show(bool show)
{
  m_pBoard->Show(show);
  m_pStatus->Show(show);
  if (show)
  {
    Layout();
    m_pBoard->start();
  }
  return wxPanel::Show(show);
}

#pragma warning(push)
#pragma warning(disable : 4351) // new behavior - elements of array
                                // 'Board::m_board' will be default initialized
Board::Board(wxWindow* pParent, wxStatusBar* const pStatusBar)
  : wxPanel(pParent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxBORDER_NONE | wxWANTS_CHARS),
    m_bufferBitmap(),
    m_timer(),
    m_isFallingFinished(false),
    m_curShape(),
    m_curX(0),
    m_curY(0),
    m_numLinesRemoved(0),
    m_board(),
    m_pStatusBar(pStatusBar)
{
  m_timer.SetOwner(this, m_timer.GetId());
  Connect(wxEVT_PAINT, wxPaintEventHandler(Board::onPaint));
  Connect(
    wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(Board::onEraseBackground));
  Connect(wxEVT_SIZE, wxSizeEventHandler(Board::onSize));
  Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(Board::onKeyDown));
  Connect(wxEVT_TIMER, wxCommandEventHandler(Board::onTimer));
}
#pragma warning(pop)

void Board::start()
{
  srand(time(NULL));
  recreateBuffer();
  m_isFallingFinished = false;
  m_numLinesRemoved = 0;
  if (m_pStatusBar) m_pStatusBar->SetStatusText("0");
  clearBoard();
  newPiece();
  m_timer.Start(ms_timerInterval);
  SetFocus();
}

void Board::onPaint(wxPaintEvent& WXUNUSED(rEvent))
{
  wxBufferedPaintDC dc(this, m_bufferBitmap);
  wxSize size = GetClientSize();
  // paint background - see comment in onEraseBackground method to see why
  wxColour bgc = GetBackgroundColour();
  if (!bgc.Ok()) bgc = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
  dc.SetBrush(wxBrush(bgc));
  dc.SetPen(wxPen(bgc, 1));
  dc.DrawRectangle(size);
  // paint all the shapes, or remains of the shapes, that have been dropped to
  // the bottom of the board; all the squares are remembered in the m_board
  // array - we access this array using the shapeAt() method
  int boardTop = size.GetHeight() - eBOARD_HEIGHT * squareHeight();
  for (int i = 0; i < eBOARD_HEIGHT; ++i)
  {
    for (int j = 0; j < eBOARD_WIDTH; ++j)
    {
      Tetrominoes_e shape = shapeAt(j, eBOARD_HEIGHT - i - 1);
      if (eSHAPE_0 != shape)
        drawSquare(
          dc, 0 + j * squareWidth(), boardTop + i * squareHeight(), shape);
    }
  }
  // paint the shape that is falling down
  if (eSHAPE_0 != m_curShape.getShape())
  {
    for (int i = 0; i < 4; ++i)
    {
      int x = m_curX + m_curShape.x(i);
      int y = m_curY - m_curShape.y(i);
      drawSquare(dc,
                 0 + x * squareWidth(),
                 boardTop + (eBOARD_HEIGHT - y - 1) * squareHeight(),
                 m_curShape.getShape());
    }
  }
}

void Board::onEraseBackground(wxEraseEvent& WXUNUSED(rEvent))
{
  // Empty implementation, to prevent flicker.
  //  We paint the background in the paint handler, so that all painting will
  //  be done in the buffered paint handler and we won't see the background
  //  being erased before the paint handler is called.
}

void Board::onSize(wxSizeEvent& WXUNUSED(rEvent))
{
  recreateBuffer();
}

void Board::onKeyDown(wxKeyEvent& rEvent)
{ // check for pressed keys...
  switch (rEvent.GetKeyCode())
  {
  case 'c':
  case 'C':
  case 's':
  case 'S':
    start(); // clear or start over
    return;
    break;
  }
  if (eSHAPE_0 == m_curShape.getShape())
  {
    rEvent.Skip();
    return;
  }
  int keycode = rEvent.GetKeyCode();
  if ('p' == keycode || 'P' == keycode || WXK_RETURN == keycode)
  {
    pause();
    return;
  }
  if (WXK_HOME == keycode)
  {
    pause();
    wxWindow* pWin =
      GetParent()->IsTopLevel()
        ? GetParent()
        : (GetGrandParent()->IsTopLevel()) ? GetGrandParent() : NULL;
    const wxEventType EVT_TETRIS = wxNewEventType();
    FunEvent evt(EVT_TETRIS, pWin ? pWin->GetId() : wxID_ANY);
    evt.SetInt(0);
    GetEventHandler()->ProcessEvent(evt);
    return;
  }
  if (!m_timer.IsRunning()) // paused
    return;
  switch (keycode)
  { // if we press the left arrow key, we try to move the piece to the left;
  // we say try, because the piece might not be able to move (something -
  // side wall, or other tetris piece - is obstructing it)
  case WXK_LEFT:
    tryMove(m_curShape, m_curX - 1, m_curY);
    break;
  case WXK_RIGHT:
    tryMove(m_curShape, m_curX + 1, m_curY);
    break;
  case WXK_DOWN:
    oneLineDown();
    break;
  case 'z':
  case 'Z':
    tryMove(m_curShape.rotateLeft(), m_curX, m_curY);
    break;
  case 'x':
  case 'X':
    tryMove(m_curShape.rotateRight(), m_curX, m_curY);
    break;
  case WXK_SPACE:
  case WXK_PAGEDOWN:
    // drops the falling shape immediately to the bottom of the board
    dropDown();
    break;
  default:
    rEvent.Skip();
  }
}

void Board::onTimer(wxCommandEvent& WXUNUSED(rEvent))
{
  wxWindow* pWin =
    GetParent()->IsTopLevel()
      ? GetParent()
      : (GetGrandParent()->IsTopLevel()) ? GetGrandParent() : NULL;
  wxTopLevelWindow* pTop = pWin ? static_cast<wxTopLevelWindow*>(pWin) : NULL;
  if (pTop && !pTop->IsActive()) pause(true);
  // we either create a new piece, after the previous one was dropped to the
  // bottom, or we move a falling piece one line down
  if (m_isFallingFinished)
  {
    m_isFallingFinished = false;
    newPiece();
  }
  else
    oneLineDown();
}

void Board::pause(bool forced)
{
  bool paused = !m_timer.IsRunning();
  if (forced)
    paused = true;
  else
    paused = !paused;
  if (paused)
  {
    wxString str;
    str.Printf(wxT("%d : paused"), m_numLinesRemoved);
    if (m_pStatusBar) m_pStatusBar->SetStatusText(str);
    m_timer.Stop();
  }
  else
  {
    wxString str;
    str.Printf(wxT("%d"), m_numLinesRemoved);
    if (m_pStatusBar) m_pStatusBar->SetStatusText(str);
    m_timer.Start(ms_timerInterval);
  }
  Refresh();
}

Tetrominoes_e& Board::shapeAt(int x, int y)
{
  return m_board[(y * eBOARD_WIDTH) + x];
}

int Board::squareWidth()
{
  return GetClientSize().GetWidth() / eBOARD_WIDTH;
}

int Board::squareHeight()
{
  return GetClientSize().GetHeight() / eBOARD_HEIGHT;
}

void Board::clearBoard()
{
  for (int i = 0; i < eBOARD_HEIGHT * eBOARD_WIDTH; ++i)
    m_board[i] = eSHAPE_0;
}

void Board::dropDown()
{ // drops the falling shape immediately to the bottom of the board
  int newY = m_curY;
  while (newY > 0)
  {
    if (!tryMove(m_curShape, m_curX, newY - 1)) break;
    --newY;
  }
  pieceDropped();
}

void Board::oneLineDown()
{
  if (!tryMove(m_curShape, m_curX, m_curY - 1)) pieceDropped();
}

void Board::pieceDropped()
{ // in this method we set the current shape at it's final position
  for (int i = 0; i < 4; ++i)
  {
    int x = m_curX + m_curShape.x(i);
    int y = m_curY - m_curShape.y(i);
    shapeAt(x, y) = m_curShape.getShape();
  }
  // call removeFullLines to check if we have at least one full line
  removeFullLines();
  if (!m_isFallingFinished) newPiece();
}

void Board::removeFullLines()
{
  int numFullLines = 0;
  for (int i = eBOARD_HEIGHT - 1; i >= 0; --i)
  {
    bool lineIsFull = true;
    for (int j = 0; j < eBOARD_WIDTH; ++j)
    {
      if (eSHAPE_0 == shapeAt(j, i))
      {
        lineIsFull = false;
        break;
      }
    }
    if (lineIsFull)
    {
      ++numFullLines;
      for (int k = i; k < eBOARD_HEIGHT - 1; ++k)
      {
        for (int j = 0; j < eBOARD_WIDTH; ++j)
          shapeAt(j, k) = shapeAt(j, k + 1);
      }
    }
  }
  if (numFullLines > 0)
  {
    m_numLinesRemoved += numFullLines;
    wxString str;
    str.Printf(wxT("%d"), m_numLinesRemoved);
    if (m_pStatusBar) m_pStatusBar->SetStatusText(str);
    m_isFallingFinished = true;
    m_curShape.setShape(eSHAPE_0);
    Refresh();
  }
}

void Board::newPiece()
{
  m_curShape.setRandomShape();
  m_curX = eBOARD_WIDTH / 2 + 1;
  m_curY = eBOARD_HEIGHT - 1 + m_curShape.minY();
  if (!tryMove(m_curShape, m_curX, m_curY))
  {
    m_curShape.setShape(eSHAPE_0);
    m_timer.Stop();
    wxString str;
    str.Printf(wxT("%d : game over"), m_numLinesRemoved);
    if (m_pStatusBar) m_pStatusBar->SetStatusText(str);
  }
}

bool Board::tryMove(const Shape& newPiece, int newX, int newY)
{
  for (int i = 0; i < 4; ++i)
  {
    int x = newX + newPiece.x(i);
    int y = newY - newPiece.y(i);
    if (x < 0 || x >= eBOARD_WIDTH || y < 0 || y >= eBOARD_HEIGHT) return false;
    if (eSHAPE_0 != shapeAt(x, y)) return false;
  }
  m_curShape = newPiece;
  m_curX = newX;
  m_curY = newY;
  Refresh();
  return true;
}

void Board::drawSquare(wxBufferedPaintDC& rDc,
                       int x,
                       int y,
                       Tetrominoes_e shape)
{
  static wxColour colors[] = {wxColour(0x00, 0x00, 0x00),
                              wxColour(0xCC, 0x66, 0x66),
                              wxColour(0x66, 0xCC, 0x66),
                              wxColour(0x66, 0x66, 0xCC),
                              wxColour(0xCC, 0xCC, 0x66),
                              wxColour(0xCC, 0x66, 0xCC),
                              wxColour(0x66, 0xCC, 0xCC),
                              wxColour(0xDA, 0xAA, 0x00)};
  static wxColour light[] = {wxColour(0x00, 0x00, 0x00),
                             wxColour(0xF8, 0x9F, 0xAB),
                             wxColour(0x79, 0xFC, 0x79),
                             wxColour(0x79, 0x79, 0xFC),
                             wxColour(0xFC, 0xFC, 0x79),
                             wxColour(0xFC, 0x79, 0xFC),
                             wxColour(0x79, 0xFC, 0xFC),
                             wxColour(0xFC, 0xC6, 0x00)};
  static wxColour dark[] = {wxColour(0x00, 0x00, 0x00),
                            wxColour(0x80, 0x3B, 0x3B),
                            wxColour(0x3B, 0x80, 0x3B),
                            wxColour(0x3B, 0x3B, 0x80),
                            wxColour(0x80, 0x80, 0x3B),
                            wxColour(0x80, 0x3B, 0x80),
                            wxColour(0x3B, 0x80, 0x80),
                            wxColour(0x80, 0x62, 0x00)};

  wxPen pen(light[int(shape)]);
  pen.SetCap(wxCAP_PROJECTING);
  rDc.SetPen(pen);
  rDc.DrawLine(x, y + squareHeight() - 1, x, y);
  rDc.DrawLine(x, y, x + squareWidth() - 1, y);

  wxPen darkpen(dark[int(shape)]);
  darkpen.SetCap(wxCAP_PROJECTING);
  rDc.SetPen(darkpen);
  rDc.DrawLine(x + 1,
               y + squareHeight() - 1,
               x + squareWidth() - 1,
               y + squareHeight() - 1);
  rDc.DrawLine(x + squareWidth() - 1,
               y + squareHeight() - 1,
               x + squareWidth() - 1,
               y + 1);

  rDc.SetPen(*wxTRANSPARENT_PEN);
  rDc.SetBrush(wxBrush(colors[int(shape)]));
  rDc.DrawRectangle(x + 1, y + 1, squareWidth() - 2, squareHeight() - 2);
}

bool Board::recreateBuffer(const wxSize& rSize)
{
  wxSize sz = rSize;
  if (wxDefaultSize == sz) sz = GetClientSize();
  if (sz.x < 1 || sz.y < 1) return false;
  if (!m_bufferBitmap.Ok() || m_bufferBitmap.GetWidth() < sz.x ||
      m_bufferBitmap.GetHeight() < sz.y)
  {
    m_bufferBitmap = wxBitmap(sz.x, sz.y);
  }
  return m_bufferBitmap.Ok();
}

Shape::Shape()
{
  setShape(eSHAPE_0);
}

void Shape::setShape(Tetrominoes_e shape)
{ // 8 shapes, formed with 4 squares, x,y coordinates (2) to specify shape
  // clang-format off
  static const int coordsTable[eSHAPE_NUM][4][2] =
  {
    { { 0, 0}, { 0, 0}, { 0, 0}, { 0, 0} },  // eSHAPE_0
    { { 0,-1}, { 0, 0}, { 0, 1}, { 0, 2} },  // eSHAPE_I
    { {-1,-1}, { 0,-1}, { 0, 0}, { 0, 1} },  // eSHAPE_J
    { { 1,-1}, { 0,-1}, { 0, 0}, { 0, 1} },  // eSHAPE_L
    { { 0, 0}, { 1, 0}, { 0, 1}, { 1, 1} },  // eSHAPE_O
    { { 0,-1}, { 0, 0}, {-1, 0}, {-1, 1} },  // eSHAPE_S
    { { 0,-1}, { 0, 0}, { 1, 0}, { 1, 1} },  // eSHAPE_Z
    { {-1, 0}, { 0, 0}, { 1, 0}, { 0, 1} }   // eSHAPE_T
  };
  // clang-format on
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 2; j++)
      m_coords[i][j] = coordsTable[shape][i][j];
  }
  m_shape = shape;
}

void Shape::setRandomShape()
{
  int x = rand() % 7 + 1;
  setShape(Tetrominoes_e(x));
}

Tetrominoes_e Shape::getShape() const
{
  return m_shape;
}

int Shape::x(int index) const
{
  return m_coords[index][0];
}

int Shape::y(int index) const
{
  return m_coords[index][1];
}

int Shape::minX() const
{
  int m = m_coords[0][0];
  for (int i = 0; i < 4; i++)
    m = std::min(m, m_coords[i][0]);
  return m;
}

int Shape::maxX() const
{
  int m = m_coords[0][0];
  for (int i = 0; i < 4; i++)
    m = std::max(m, m_coords[i][0]);
  return m;
}

int Shape::minY() const
{
  int m = m_coords[0][1];
  for (int i = 0; i < 4; i++)
    m = std::min(m, m_coords[i][1]);
  return m;
}

int Shape::maxY() const
{
  int m = m_coords[0][1];
  for (int i = 0; i < 4; i++)
    m = std::max(m, m_coords[i][1]);
  return m;
}

Shape Shape::rotateLeft() const
{
  if (eSHAPE_O == m_shape) return *this;
  Shape result;
  result.m_shape = m_shape;
  for (int i = 0; i < 4; i++)
  {
    result.setX(i, y(i));
    result.setY(i, -x(i));
  }
  return result;
}

Shape Shape::rotateRight() const
{
  if (eSHAPE_O == m_shape) return *this;
  Shape result;
  result.m_shape = m_shape;
  for (int i = 0; i < 4; i++)
  {
    result.setX(i, -y(i));
    result.setY(i, x(i));
  }
  return result;
}

void Shape::setX(int index, int x)
{
  m_coords[index][0] = x;
}

void Shape::setY(int index, int y)
{
  m_coords[index][1] = y;
}
