#pragma once

#include <QHeaderView>
#include <QBrush>
#include <QPainter>
#include <QFileInfo>
#include <QAbstractItemModel>
#include <qnamespace.h>

class FileHeader : public QHeaderView {
    public:
        explicit FileHeader(QWidget *parent = nullptr) : QHeaderView(Qt::Vertical, parent){}

    protected:
        void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override {
            QHeaderView::paintSection(painter, rect, logicalIndex);

            int firstVisible = logicalIndexAt(0);

            if (logicalIndex == firstVisible){
                QString filename = model()->headerData(logicalIndex, Qt::Vertical, Qt::UserRole).toString();

                painter->save();
                QRect textArea = rect.adjusted(6, 1, -1, -1);
                painter->fillRect(textArea, QColor("#dfe8f6"));

                painter->setPen(Qt::black);
                painter->drawText(textArea, Qt::AlignVCenter | Qt::AlignLeft, filename);
                painter->restore();
            }

            if (!model() || !rect.isValid()){
                return;
            }

            QVariant color = model()->headerData(logicalIndex, Qt::Vertical, Qt::BackgroundRole);

            if (!color.isValid()){
                return;
            }

            painter->save();

            painter->fillRect(
                QRect(rect.left() + 1, rect.top() + 1, 4, rect.height() - 2), qvariant_cast<QBrush>(color)
            );

            painter->restore();
        }
};