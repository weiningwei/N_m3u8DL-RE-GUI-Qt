#include "stringlistwidget.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

StringListWidget::StringListWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    m_list = new QListWidget(this);
    mainLayout->addWidget(m_list);

    auto *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("输入一项后点击添加...");
    row->addWidget(m_input);

    auto *addBtn = new QPushButton("添加", this);
    auto *delBtn = new QPushButton("删除选中", this);
    row->addWidget(addBtn);
    row->addWidget(delBtn);

    mainLayout->addLayout(row);

    connect(addBtn, &QPushButton::clicked, this, &StringListWidget::addItem);
    connect(delBtn, &QPushButton::clicked, this, &StringListWidget::removeSelected);
    connect(m_input, &QLineEdit::returnPressed, this, &StringListWidget::addItem);
}

QStringList StringListWidget::items() const
{
    QStringList result;
    for (int i = 0; i < m_list->count(); ++i)
        result.append(m_list->item(i)->text());
    return result;
}

void StringListWidget::setItems(const QStringList &items)
{
    m_list->clear();
    for (const QString &item : items)
        m_list->addItem(item);
}

void StringListWidget::clear()
{
    m_list->clear();
}

void StringListWidget::addItem()
{
    const QString text = m_input->text().trimmed();
    if (text.isEmpty())
        return;
    m_list->addItem(text);
    m_input->clear();
    emit itemsChanged();
}

void StringListWidget::removeSelected()
{
    const QList<QListWidgetItem *> selected = m_list->selectedItems();
    for (QListWidgetItem *item : selected)
        delete item;
    if (!selected.isEmpty())
        emit itemsChanged();
}
