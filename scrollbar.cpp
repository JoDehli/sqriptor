/*
 *   Sqriptor text editor for Qt5
 *   Copyright 2021 by Thomas Lübking <thomas.luebking@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QStyle>
#include <QStyleOptionSlider>
#include "scrollbar.h"

#include <QDebug>

ScrollBar::ScrollBar() : QScrollBar() {}

void ScrollBar::paintEvent(QPaintEvent *event)
{
    QScrollBar::paintEvent(event);
    if (maximum() < 1)
        return;
//        Q_D(QScrollBar);
    QPainter p(this);
    p.setPen(Qt::white);
    p.setCompositionMode(QPainter::RasterOp_SourceXorDestination); // QPainter::RasterOp_NotSourceXorDestination
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
//    style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &p, this);
    QRect groove = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, this);
    QRect slider = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);
    groove.adjust(  groove.width()/3, slider.height()/2,
                   -groove.width()/3, slider.height()/2 - slider.height());
    foreach (int marker, m_markers) {
        int y = groove.y() + groove.height()*marker/maximum();
        p.drawLine(groove.left(), y, groove.right(), y);
    }
}