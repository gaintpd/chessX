/***************************************************************************
                      BoardView - view of the current board
                             -------------------
    begin                : Sun 21 Aug 2005
    copyright            : (C) 2005 Michal Rudolf <mrudolf@kdewebdev.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boardview.h"
#include "settings.h"

#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QWheelEvent>

using namespace Qt;

BoardView::BoardView(QWidget* parent, int flags) : QWidget(parent),
   m_flipped(false), m_showFrame(false), m_selectedSquare(InvalidSquare), m_flags(flags),
   m_dragged(Empty), m_clickUsed(false)
{
}

BoardView::~BoardView()
{
}

void BoardView::setFlags(int flags)
{
  m_flags = flags;
}

void BoardView::setBoard(const Board& value)
{
  m_clickUsed = true;
  Board oldboard = m_board;
  m_board = value;
  update();
}

Board BoardView::board() const
{
  return m_board;
}

const BoardTheme& BoardView::theme() const
{
  return m_theme;
}

void BoardView::paintEvent(QPaintEvent* event)
{
  QPainter p(this);
  // Draw squares
  for (Square square = 0; square < 64; square++)
  {
    QRect rect = squareRect(square);
    if (!event->region().intersects(rect))
      continue;
    int x = isFlipped() ? 7 - square % 8 : square % 8;
    int y = isFlipped() ? square / 8 : 7 - square / 8;
    QPoint pos(x * m_theme.size().width(), y * m_theme.size().height());
    p.drawPixmap(pos, m_theme.square((x + y) % 2));
    p.drawPixmap(pos, m_theme.piece(m_board.at(square)));
    if (square == m_selectedSquare)
    {
      QPen pen;
      pen.setColor(QColor(Qt::yellow));
      pen.setWidth(2);
      p.setPen(pen);
      p.drawRect(pos.x() + 1 + m_showFrame, pos.y() + 1 + m_showFrame,
         m_theme.size().width() - 2 - m_showFrame, m_theme.size().height() - 2 - m_showFrame);
     }
     if (m_showFrame)
     {
       p.setPen(QColor(Qt::black));
       p.drawRect(QRect(pos, m_theme.size()));
     }
  }
  // Draw dragged piece
  if (m_dragged != Empty)
    p.drawPixmap(m_dragPoint, m_theme.piece(m_dragged));
  // Draw side to move indicator
  bool white = m_board.toMove() == White;
  int square = m_theme.size().width() / 3;
  QColor color = white ? Qt::white : Qt::black;
  QColor border = white ? Qt::black : Qt::white;
  int posy = (white == m_flipped) ? 1 : 8 * m_theme.size().width() - square;
  p.setPen(border);
  p.setBrush(QColor(color));
  p.drawRect(8 * m_theme.size().width() + 8, posy, square, square);
}

void BoardView::resizeBoard()
{
  // subtract move indicator from width
  int xsize = (width() - (8 + width() / 24) - 1) / 8;
  int ysize = (height() - 1) / 8;
  int size = xsize < ysize ? xsize : ysize;
  m_theme.setSize(QSize(size, size));
}

void BoardView::resizeEvent(QResizeEvent*)
{
  resizeBoard();
}

Square BoardView::squareAt(const QPoint& p) const
{
  int x = isFlipped() ? 7 - p.x() / m_theme.size().width() : p.x() / m_theme.size().width();
  int y = isFlipped() ? p.y() / m_theme.size().height() : 7 - p.y() / m_theme.size().height();
  if (x >= 0 && x < 8 && y >= 0 && y <= 8)
    return 8 * y + x;
  else return InvalidSquare;
}

void BoardView::mousePressEvent(QMouseEvent* event)
{
    m_dragStart = event->pos();
}

void BoardView::mouseMoveEvent(QMouseEvent *event)
{
  if (!(event->buttons() & Qt::LeftButton))
    return;
  if (m_dragged != Empty)
  {
    QRect old = QRect(m_dragPoint, m_theme.size());
    m_dragPoint = event->pos() - m_theme.pieceCenter();
    update(old);
    update(QRect(m_dragPoint, m_theme.size()));
    return;
  }
  if ((event->pos() - m_dragStart).manhattanLength()
      < QApplication::startDragDistance())  // Click and move - start dragging
    return;
  Square s = squareAt(m_dragStart);
  if (!canDrag(s))
    return;
  m_dragged = m_board.at(s);
  m_dragPoint = event->pos() - m_theme.pieceCenter();
  m_board.removeFrom(s);
  update(squareRect(s));
  update(QRect(m_dragPoint, m_theme.size()));
  unselectSquare();
  QPixmap icon = m_theme.piece(m_dragged);
}

void BoardView::mouseReleaseEvent(QMouseEvent* event)
{
  Square s = squareAt(event->pos());
  m_clickUsed = false;
  if (event->button() != Qt::LeftButton) {
       if (s != InvalidSquare)
               emit clicked(s, event->button() + event->modifiers());
       return;
  }

  if (m_dragged != Empty)
  {
    Square from = squareAt(m_dragStart);
    m_board.setAt(from, m_dragged);
    QRect oldr = QRect(m_dragPoint, m_theme.size());
    m_dragged = Empty;
    update(squareRect(from));
    update(oldr);
    if (s != InvalidSquare)
      emit moveMade(from, s);
  }
  else if (m_selectedSquare != InvalidSquare)
  {
    Square from = m_selectedSquare;
    unselectSquare();
    if (s != InvalidSquare)
      emit moveMade(from, s);
  }
  else {
         if (s != InvalidSquare)
                 emit clicked(s, event->button() + event->modifiers());
         if (!m_clickUsed && isPieceColor(m_board.at(s), m_board.toMove()))
                 selectSquare(s);
  }
}

void BoardView::wheelEvent(QWheelEvent* e)
{
  int change = e->delta() < 0 ? WheelDown : WheelUp;
  emit wheelScrolled(change + e->modifiers());
}

bool BoardView::setTheme(const QString& pieceFile, const QString& boardFile)
{
  bool result = m_theme.load(pieceFile, boardFile);
  if (!result)
  {
    QMessageBox::warning(0, tr("Error"), tr("<qt>Cannot open theme <b>%1</b> from directory:<br>%2</qt>")
        .arg(pieceFile).arg(m_theme.themeDirectory()));
    // If there is no theme, try to load default
    if (!m_theme.isValid())
    {
      result = m_theme.load("default");
      if (result)
        resizeBoard();
    }
  }
  if (result)
    update();
  return result;
}

void BoardView::flip()
{
  m_flipped = !m_flipped;
  update();
}

bool BoardView::isFlipped() const
{
  return m_flipped;
}

void BoardView::configure()
{
  AppSettings->beginGroup("/Board/");
  m_showFrame = AppSettings->value("showFrame", true).toBool();
  m_theme.setSquareType(BoardTheme::BoardSquare(AppSettings->value("squareType", 0).toInt()));
  m_theme.setLightColor(AppSettings->value("lightColor", "#d0d0d0").value<QColor>());
  m_theme.setDarkColor(AppSettings->value("darkColor", "#a0a0a0").value<QColor>());
  QString pieceTheme = AppSettings->value("pieceTheme", "default").toString();
  QString boardTheme = AppSettings->value("boardTheme", "default").toString();
  setTheme(pieceTheme, boardTheme);
  AppSettings->endGroup();
  update();
}

void BoardView::selectSquare(Square s)
{
  if (m_selectedSquare == s)
    return;
  unselectSquare();
  m_selectedSquare = s;
  update(squareRect(s));
}

void BoardView::unselectSquare()
{
  Square prev = m_selectedSquare;
  m_selectedSquare = InvalidSquare;
  if (prev != InvalidSquare)
    update(squareRect(prev));
}

QRect BoardView::squareRect(Square square)
{
  int x = isFlipped() ? 7 - square % 8 : square % 8;
  int y = isFlipped() ? square / 8 : 7 - square / 8;
  return QRect(QPoint(x * m_theme.size().width(), y * m_theme.size().height()), 
               m_theme.size());
}

bool BoardView::canDrag(Square s) const
{
  if (m_dragged != Empty) // already dragging
    return false;
  if (s == InvalidSquare)
    return false;
  else if (m_flags & IgnoreSideToMove)
    return m_board.at(s) != Empty;
  else return isPieceColor(m_board.at(s), m_board.toMove());
}

