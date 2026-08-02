#pragma once

#include <QWidget>
#include <QStringList>

// A small editor for repeatable command-line options such as
// request headers (-H), decryption keys (--key) and mux imports
// (--mux-import). Each entry is a raw string that is emitted as a
// separate occurrence of the owning flag.
class StringListWidget : public QWidget {
    Q_OBJECT

public:
    explicit StringListWidget(QWidget *parent = nullptr);

    QStringList items() const;
    void setItems(const QStringList &items);
    void clear();

signals:
    void itemsChanged();

private slots:
    void addItem();
    void removeSelected();

private:
    class QListWidget *m_list = nullptr;
    class QLineEdit *m_input = nullptr;
};
