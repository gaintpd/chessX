/****************************************************************************
*   Copyright (C) 2013 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#include "boardview.h"
#include "common.h"
#include "promotiondialog.h"
#include "ui_promotiondialog.h"

PromotionDialog::PromotionDialog(QWidget *parent, Color color) :
    QDialog(parent),
    ui(new Ui::PromotionDialog),
    m_index(-1)
{
    BoardView view;
    view.configure();
    ui->setupUi(this);
    int offset = color==White ? 0 : (BlackKing-WhiteKing);
    ui->btQueen->setIcon(view.theme().piece(Piece(WhiteQueen+offset)));
    ui->btRook->setIcon(view.theme().piece(Piece(WhiteRook+offset)));
    ui->btBishop->setIcon(view.theme().piece(Piece(WhiteBishop+offset)));
    ui->btKnight->setIcon(view.theme().piece(Piece(WhiteKnight+offset)));
}

PromotionDialog::~PromotionDialog()
{
    delete ui;
}

void PromotionDialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
}

void PromotionDialog::resizeEvent(QResizeEvent *)
{
    move(m_pos);
}


int PromotionDialog::getIndex()
{
    m_pos = mapFromGlobal(QCursor::pos());
    move(m_pos);
    exec();
    return m_index;
}

void PromotionDialog::setIndex()
{
    QToolButton* pButton = qobject_cast<QToolButton*>(sender());
    if (pButton == ui->btQueen)
    {
        m_index = 0;
    }
    else if (pButton == ui->btRook)
    {
        m_index = 1;
    }
    else if (pButton == ui->btBishop)
    {
        m_index = 2;
    }
    else if (pButton == ui->btKnight)
    {
        m_index = 3;
    }
    accept();
}
